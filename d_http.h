// wengwengweng

#ifndef D_HTTP_H
#define D_HTTP_H

typedef enum {
	D_HTTP_GET,
	D_HTTP_POST,
	D_HTTP_PUT,
	D_HTTP_DELETE,
	D_HTTP_HEAD,
	D_HTTP_CONNECT,
	D_HTTP_OPTIONS,
	D_HTTP_TRACE,
	D_HTTP_PATCH,
} d_http_method;

typedef struct {
	int start;
	int end;
	const char *msg;
} d_http_str;

typedef struct {
	d_http_str key;
	d_http_str val;
} d_http_header;

typedef struct {
	int status;
	char *msg;
	int num_headers;
	d_http_header *headers;
	d_http_str body;
} d_http_response;

typedef struct {
	d_http_method method;
} d_http_request;

typedef struct {
	int sock_fd;
} d_http_server;

typedef struct {
	int sock_fd;
} d_http_client;

typedef char *(*d_http_handler)(const char*);

void d_http_serve(int port, d_http_handler handler);
d_http_server d_http_make_server(int port);
void d_http_free_server(d_http_server *server);
void d_http_server_listen(const d_http_server *server, d_http_handler handler);

char *d_http_fetch(const char *host, const char *msg);
d_http_client d_http_make_client(const char *host);
void d_http_free_client(d_http_client *client);
char *d_http_client_send(const d_http_client *client, const char *msg);

#endif

#ifdef D_IMPL
#define D_HTTP_IMPL
#endif

#ifdef D_HTTP_IMPL
#ifndef D_HTTP_IMPL_ONCE
#define D_HTTP_IMPL_ONCE

#include <stdbool.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHUNK 1024
#define HTTP_PORT 80

static char const *status_text[] = {
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",

	//100s
	"Continue", "Switching Protocols", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",

	//200s
	"OK", "Created", "Accepted", "Non-Authoritative Information", "No Content",
	"Reset Content", "Partial Content", "", "", "",

	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",

	//300s
	"Multiple Choices", "Moved Permanently", "Found", "See Other", "Not Modified",
	"Use Proxy", "", "Temporary Redirect", "", "",

	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",

	//400s
	"Bad Request", "Unauthorized", "Payment Required", "Forbidden", "Not Found",
	"Method Not Allowed", "Not Acceptable", "Proxy Authentication Required",
	"Request Timeout", "Conflict",

	"Gone", "Length Required", "", "Payload Too Large", "", "", "", "", "", "",

	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",

	//500s
	"Internal Server Error", "Not Implemented", "Bad Gateway", "Service Unavailable",
	"Gateway Timeout", "", "", "", "", ""

	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "",
};

d_http_server d_http_make_server(int port) {

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1) {
		fprintf(stderr, "failed to create socket\n");
	}

	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));

	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_addr = {
			.s_addr = INADDR_ANY,
		},
		.sin_port = htons(port),
	};

	if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		switch (errno) {
			case EACCES:
				fprintf(stderr, "port %d is in protected\n", port);
				break;
			case EADDRINUSE:
				fprintf(stderr, "port %d is in use\n", port);
				break;
			default:
				fprintf(stderr, "failed to bind socket\n");
				break;
		}
	}

	listen(sock_fd, 64);

	return (d_http_server) {
		.sock_fd = sock_fd,
	};

}

void d_http_free_server(d_http_server *server) {
	close(server->sock_fd);
}

void d_http_server_listen(const d_http_server *server, d_http_handler handler) {

	int conn_fd = accept(server->sock_fd, NULL, NULL);
	char *req_msg = malloc(CHUNK);
	int iter = 0;

	while (read(conn_fd, req_msg + iter * CHUNK, CHUNK) >= CHUNK) {
		iter++;
		req_msg = realloc(req_msg, (iter + 1) * CHUNK);
	}

	char *res_msg = handler(req_msg);

	write(conn_fd, res_msg, strlen(res_msg));
	free(req_msg);
	close(conn_fd);

}

void d_http_serve(int port, d_http_handler handler) {

	d_http_server server = d_http_make_server(port);

	while (1) {
		d_http_server_listen(&server, handler);
	}

	d_http_free_server(&server);

}

d_http_client d_http_make_client(const char *host) {

	struct addrinfo *res;

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_flags = AI_PASSIVE | AI_CANONNAME,
	};

	getaddrinfo(host, "http", &hints, &res);

	int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	if (sock_fd == -1) {
		fprintf(stderr, "failed to create socket\n");
	}

	connect(sock_fd, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return (d_http_client) {
		.sock_fd = sock_fd,
	};

}

void d_http_free_client(d_http_client *client) {
	close(client->sock_fd);
}

char *d_http_client_send(const d_http_client *client, const char *req_msg) {

	write(client->sock_fd, req_msg, strlen(req_msg));
	int iter = 0;
	char *res_msg = malloc(CHUNK);
	int total_read = 0;
	int content_len = 0;
	int body_pos = 0;

	while (1) {

		total_read += read(client->sock_fd, res_msg + total_read, CHUNK);
		// TODO: only realloc when short
		iter++;
		res_msg = realloc(res_msg, (iter + 1) * CHUNK);

		// find content length
		if (content_len == 0) {
			char *length_pos = strstr(res_msg, "Content-Length");
			if (length_pos) {
				char *start = strstr(length_pos, ":") + 1;
				while (start[0] == ' ') {
					start++;
				}
				char *end = strstr(start, "\r\n");
				for (int i = 0; i < end - start; i++) {
					content_len = content_len * 10 + (start[i] - '0');
				}
			}
		}

		// find body
		if (body_pos == 0) {
			char *body = strstr(res_msg, "\r\n\r\n");
			if (body) {
				body_pos = body + 4 - res_msg;
			}
		}

		if (body_pos != 0) {
			if (total_read - body_pos >= content_len) {
				break;
			}
		}

	}

	res_msg[total_read] = '\0';

	return res_msg;

}

char *d_http_fetch(const char *host, const char *req_msg) {
	d_http_client client = d_http_make_client(host);
	char *res_msg = d_http_client_send(&client, req_msg);
	d_http_free_client(&client);
	return res_msg;
}

#endif
#endif

