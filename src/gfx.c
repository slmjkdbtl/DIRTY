// wengwengweng

#include <ctype.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <dirty/dirty.h>
#include "gfx.h"
#include "utils.h"
#include "res/unscii.png.h"

#define T_STACKS 16
#define TEX_SLOTS 4
#define NEAR -1024.0
#define FAR 1024.0

static const char *vert_template = GLSL(

	attribute vec3 a_pos;
	attribute vec3 a_normal;
	attribute vec2 a_uv;
	attribute vec4 a_color;

	varying vec3 v_pos;
	varying vec3 v_normal;
	varying vec2 v_uv;
	varying vec4 v_color;

	uniform mat4 u_model;
	uniform mat4 u_view;
	uniform mat4 u_proj;

	vec4 default_pos() {
		return u_proj * u_view * u_model * vec4(v_pos, 1.0);
	}

	##USER##

	void main() {
		v_pos = a_pos;
		v_uv = a_uv;
		v_color = a_color;
		v_normal = normalize(a_normal);
		gl_Position = vert();
	}

);

static const char *frag_template = GLSL(

	varying vec3 v_pos;
	varying vec3 v_normal;
	varying vec2 v_uv;
	varying vec4 v_color;
	uniform sampler2D u_tex;
	uniform vec4 u_color;

	vec4 default_color() {
		return v_color * u_color * texture2D(u_tex, v_uv);
	}

	##USER##

	void main() {
		gl_FragColor = frag();
		if (gl_FragColor.a == 0.0) {
			discard;
		}
	}

);

static const char *vert_default = STR(
	vec4 vert() {
		return default_pos();
	}
);

static const char *frag_default = STR(
	vec4 frag() {
		return default_color();
	}
);

static void gl_check_errors() {

	GLuint err = glGetError();

	while (err != GL_NO_ERROR) {

		switch (err) {
			case GL_INVALID_ENUM:
				d_fail("gl invalid enum\n");
				break;
			case GL_INVALID_VALUE:
				d_fail("gl invalid value\n");
				break;
			case GL_INVALID_OPERATION:
				d_fail("gl invalid operation\n");
				break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:
				d_fail("gl invalid framebuffer operation\n");
				break;
			case GL_OUT_OF_MEMORY:
				d_fail("gl out of memory\n");
				break;
			case GL_STACK_UNDERFLOW:
				d_fail("gl stack underflow\n");
				break;
			case GL_STACK_OVERFLOW:
				d_fail("gl stack overflow\n");
				break;
		}

		err = glGetError();

	}

}

typedef struct {
	d_tex default_tex;
	const d_tex *tex_slots[TEX_SLOTS];
	d_font default_font;
	const d_font *cur_font;
	d_shader default_shader;
	const d_shader *cur_shader;
	mat4 transform;
	d_cam default_cam;
	const d_cam *cur_cam;
	const d_canvas *cur_canvas;
	mat4 t_stack[T_STACKS];
	int t_stack_cnt;
	d_batch batch;
} d_gfx_t;

static d_gfx_t d_gfx;

void d_gfx_init() {

// 	const GLubyte *gl_ver = glGetString(GL_VERSION);
// 	printf("%s\n", gl_ver);

	// init gl
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glStencilFunc(GL_EQUAL, 1, 0xff);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// init default shader
	d_gfx.default_shader = d_make_shader(NULL, NULL);
	d_gfx.cur_shader = &d_gfx.default_shader;

	// init default tex
	d_gfx.default_tex = d_make_tex((unsigned char[]){255, 255, 255, 255}, 1, 1);
	d_gfx.tex_slots[0] = &d_gfx.default_tex;

	// init default font
	d_gfx.default_font = d_make_font(d_parse_tex(unscii_png, unscii_png_len), 8, 8, " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
	d_gfx.cur_font = &d_gfx.default_font;

	// init default cam
	d_gfx.default_cam.view = mat4u();
	d_gfx.default_cam.proj = mat4_ortho(d_width(), d_height(), NEAR, FAR);
	d_gfx.cur_cam = &d_gfx.default_cam;

	// init batched renderer
	d_gfx.batch = d_make_batch();

	// init transform
	d_gfx.transform = mat4u();

}

void d_gfx_frame_begin() {
	d_gfx.transform = mat4u();
	d_gfx.default_cam.proj = mat4_ortho(d_width(), d_height(), NEAR, FAR);
	d_gfx.cur_cam = &d_gfx.default_cam;
	d_clear();
	glViewport(0, 0, d_width(), d_height());
}

void d_gfx_frame_end() {
	d_batch_flush(&d_gfx.batch);
	gl_check_errors();
}

d_mesh d_make_mesh(
	const d_vertex *verts,
	int vcount,
	const d_index *indices,
	int icount
) {

	// vertex buffer
	GLuint vbuf;

	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, vcount * sizeof(d_vertex), verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// index buffer
	GLuint ibuf;

	glGenBuffers(1, &ibuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, icount * sizeof(d_index), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return (d_mesh) {
		.vbuf = vbuf,
		.ibuf = ibuf,
		.count = icount,
	};

}

void d_free_mesh(d_mesh *m) {
	glDeleteBuffers(1, &m->vbuf);
	glDeleteBuffers(1, &m->ibuf);
	m->vbuf = 0;
	m->ibuf = 0;
}

d_batch d_make_batch() {

	// vertex buffer
	GLuint vbuf;

	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, BATCH_VERT_COUNT * sizeof(d_vertex), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// index buffer
	GLuint ibuf;

	glGenBuffers(1, &ibuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, BATCH_INDEX_COUNT * sizeof(d_index), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return (d_batch) {
		.vbuf = vbuf,
		.ibuf = ibuf,
		.vqueue = {0},
		.iqueue = {0},
		.vcount = 0,
		.icount = 0,
	};

}

void d_batch_flush(d_batch *m) {

	glBindBuffer(GL_ARRAY_BUFFER, m->vbuf);
	glBufferSubData(GL_ARRAY_BUFFER, 0, m->vcount * sizeof(d_vertex), &m->vqueue);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m->ibuf);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m->icount * sizeof(d_index), &m->iqueue);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	d_push();
	d_gfx.transform = mat4u();
	d_draw(m->vbuf, m->ibuf, m->icount);
	d_pop();

	m->vcount = 0;
	m->icount = 0;
	memset(m->vqueue, 0, sizeof(m->vqueue));
	memset(m->iqueue, 0, sizeof(m->iqueue));

}

void d_batch_push(
	d_batch *m,
	const d_vertex *verts,
	int vcount,
	const d_index *indices,
	int icount
) {

	if (m->vcount + vcount >= BATCH_VERT_COUNT || m->icount + icount >= BATCH_INDEX_COUNT) {
		d_batch_flush(m);
	}

	memcpy(&m->vqueue[m->vcount], verts, vcount * sizeof(d_vertex));
	memcpy(&m->iqueue[m->icount], indices, icount * sizeof(d_index));

	for (int i = 0; i < icount; i++) {
		m->iqueue[m->icount + i] += m->vcount;
	}

	m->vcount += vcount;
	m->icount += icount;

}

void d_free_batch(d_batch *m) {
	glDeleteBuffers(1, &m->vbuf);
	glDeleteBuffers(1, &m->ibuf);
	m->vbuf = 0;
	m->ibuf = 0;
}

d_tex_conf d_default_tex_conf() {
	return (d_tex_conf) {
		.filter = D_NEAREST,
		.wrap = D_CLAMP_TO_BORDER,
	};
}

d_img d_make_img(const unsigned char *pixels, int width, int height) {
	int size = sizeof(unsigned char) * width * height * 4;
	unsigned char *data = malloc(size);
	memcpy(data, pixels, size);
	return (d_img) {
		.data = data,
		.width = width,
		.height = height,
	};
}

d_img d_parse_img(const unsigned char *bytes, int size) {

	int w, h;
	// TODO: stand alone img doesn't need this
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load_from_memory((unsigned char*)bytes, size, &w, &h, NULL, 4);

	return (d_img) {
		.data = data,
		.width = w,
		.height = h,
	};

}

d_img d_load_img(const char *path) {

	int size;
	unsigned char *bytes = d_fread_b(path, &size);
	d_img img = d_parse_img(bytes, size);
	free(bytes);

	return img;
}

void d_img_write(d_img *img, const char *path) {
	stbi_flip_vertically_on_write(true);
	stbi_write_png(path, img->width, img->height, 4, img->data, img->width * 4);
}

void d_free_img(d_img *img) {
	free(img->data);
	img->data = NULL;
}

d_tex d_make_tex_ex(const unsigned char *data, int w, int h, d_tex_conf conf) {

	GLuint tex;

	glGenTextures(1, &tex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGBA,
		w,
		h,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		data
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, conf.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, conf.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, conf.wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, conf.wrap);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (d_tex) {
		.id = tex,
		.width = w,
		.height = h,
	};

}

d_tex d_make_tex(const unsigned char *data, int w, int h) {
	return d_make_tex_ex(data, w, h, d_default_tex_conf());
}

d_tex d_img_tex_ex(const d_img *img, d_tex_conf conf) {
	return d_make_tex_ex(img->data, img->width, img->height, conf);
}

d_tex d_img_tex(const d_img *img) {
	return d_img_tex_ex(img, d_default_tex_conf());
}

d_tex d_parse_tex_ex(const unsigned char *bytes, int size, d_tex_conf conf) {

	d_img img = d_parse_img(bytes, size);
	d_tex tex = d_img_tex_ex(&img, conf);
	d_free_img(&img);

	return tex;

}

d_tex d_parse_tex(const unsigned char *bytes, int size) {
	return d_parse_tex_ex(bytes, size, d_default_tex_conf());
}

d_tex d_load_tex_ex(const char *path, d_tex_conf conf) {

	d_img img = d_load_img(path);
	d_tex tex = d_img_tex_ex(&img, conf);
	d_free_img(&img);

	return tex;
}

d_tex d_load_tex(const char *path) {
	return d_load_tex_ex(path, d_default_tex_conf());
}

void d_free_tex(d_tex *t) {
	glDeleteTextures(1, &t->id);
	t->id = 0;
}

d_font d_make_font(d_tex tex, int gw, int gh, const char *chars) {

	d_font f = {0};

	int cols = tex.width / gw;
	int rows = tex.height / gh;
	int count = cols * rows;

	f.qw = 1.0 / cols;
	f.qh = 1.0 / rows;
	f.tex = tex;

	if (count != strlen(chars)) {
		d_fail("invalid font\n");
	}

	for (int i = 0; i < count; i++) {
		f.map[(int)chars[i]] = (vec2) {
			.x = (i % cols) * f.qw,
			.y = (i / cols) * f.qh,
		};
	}

	return f;

}

void d_free_font(d_font *f) {
	d_free_tex(&f->tex);
}

d_shader d_make_shader(const char *vs_src, const char *fs_src) {

	if (vs_src == NULL) {
		vs_src = vert_default;
	}

	if (fs_src == NULL) {
		fs_src = frag_default;
	}

	const char *vs_code = strsub(vert_template, "##USER##", vs_src);
	const char *fs_code = strsub(frag_template, "##USER##", fs_src);

	GLchar info_log[512];
	GLint success = 0;

	// vertex shader
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vs, 1, &vs_code, 0);
	glCompileShader(vs);

	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE) {
		glGetShaderInfoLog(vs, 512, NULL, info_log);
		d_fail("%s", info_log);
	}

	// fragment shader
	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fs, 1, &fs_code, 0);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &success);

	if (success == GL_FALSE) {
		glGetShaderInfoLog(fs, 512, NULL, info_log);
		d_fail("%s", info_log);
	}

	// program
	GLuint program = glCreateProgram();

	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glBindAttribLocation(program, 0, "a_pos");
	glBindAttribLocation(program, 1, "a_normal");
	glBindAttribLocation(program, 2, "a_uv");
	glBindAttribLocation(program, 3, "a_color");

	glLinkProgram(program);

	glDetachShader(program, vs);
	glDetachShader(program, fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (success == GL_FALSE) {
		glGetProgramInfoLog(program, 512, NULL, info_log);
		d_fail("%s", info_log);
	}

	free((void*)vs_code);
	free((void*)fs_code);

	return (d_shader) {
		.id = program,
	};

}

d_shader d_load_shader(const char *vs_path, const char *fs_path) {

	char *vs_src = d_fread(vs_path, NULL);
	char *fs_src = d_fread(fs_path, NULL);

	d_shader s = d_make_shader(vs_src, fs_src);

	free(vs_src);
	free(fs_src);

	return s;

}

void d_free_shader(d_shader *p) {
	glDeleteProgram(p->id);
	p->id = 0;
}

d_canvas d_make_canvas(int w, int h) {
	return d_make_canvas_ex(w, h, d_default_tex_conf());
}

d_canvas d_make_canvas_ex(int w, int h, d_tex_conf conf) {

	unsigned char *buf = calloc(w * h * 4, sizeof(unsigned char));
	d_tex ctex = d_make_tex_ex(buf, w, h, conf);
	free(buf);

	GLuint dstex;
	glGenTextures(1, &dstex);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, dstex);

	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_DEPTH24_STENCIL8,
		w,
		h,
		0,
		GL_DEPTH_STENCIL,
		GL_UNSIGNED_INT_24_8,
		NULL
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, conf.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, conf.filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, conf.wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, conf.wrap);

	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint fbuf;
	glGenFramebuffers(1, &fbuf);

	glBindFramebuffer(GL_FRAMEBUFFER, fbuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ctex.id, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, dstex, 0);
	d_clear();

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		d_fail("failed to create framebuffer\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return (d_canvas) {
		.fbuf = fbuf,
		.ctex = ctex,
		.dstex = dstex,
		.width = w,
		.height = h,
	};

}

d_img d_canvas_capture(const d_canvas *c) {
	unsigned char *buf = calloc(c->width * c->height * 4, sizeof(unsigned char));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, c->ctex.id);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
	glBindTexture(GL_TEXTURE_2D, 0);
	// TODO: don't copy data here?
	d_img img = d_make_img(buf, c->width, c->height);
	free(buf);
	return img;
}

void d_free_canvas(d_canvas *c) {
	glDeleteFramebuffers(1, &c->fbuf);
	glDeleteTextures(1, &c->dstex);
	c->fbuf = 0;
	c->dstex = 0;
	d_free_tex(&c->ctex);
}

void d_send_f(const char *name, float v) {
	glUniform1f(glGetUniformLocation(d_gfx.cur_shader->id, name), v);
}

void d_send_vec2(const char *name, vec2 v) {
	glUniform2f(glGetUniformLocation(d_gfx.cur_shader->id, name), v.x, v.y);
}

void d_send_vec3(const char *name, vec3 v) {
	glUniform3f(glGetUniformLocation(d_gfx.cur_shader->id, name), v.x, v.y, v.z);
}

void d_send_color(const char *name, color c) {
	glUniform4f(glGetUniformLocation(d_gfx.cur_shader->id, name), c.r, c.g, c.b, c.a);
}

void d_send_mat4(const char *name, mat4 m) {
	glUniformMatrix4fv(glGetUniformLocation(d_gfx.cur_shader->id, name), 1, GL_FALSE, &m.m[0]);
}

void d_send_tex(const char *name, int idx, const d_tex *tex) {
	if (tex) {
		glUniform1i(glGetUniformLocation(d_gfx.cur_shader->id, name), idx + 1);
	}
	d_gfx.tex_slots[idx + 1] = tex;
}

void d_clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void d_clear_color() {
	glClear(GL_COLOR_BUFFER_BIT);
}

void d_clear_depth() {
	glClear(GL_DEPTH_BUFFER_BIT);
}

void d_clear_stencil() {
	glClear(GL_STENCIL_BUFFER_BIT);
}

void d_depth_write(bool b) {
	glDepthMask(b);
}

void d_depth_test(bool b) {
	if (b) {
		glEnable(GL_DEPTH_TEST);
	} else {
		glDisable(GL_DEPTH_TEST);
	}
}

void d_stencil_write(bool b) {
	if (b) {
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	} else {
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}
}

void d_stencil_test(bool b) {
	if (b) {
		glEnable(GL_STENCIL_TEST);
	} else {
		glDisable(GL_STENCIL_TEST);
	}
}

void d_push() {

	if (d_gfx.t_stack_cnt >= T_STACKS) {
		d_fail("transform stack overflow\n");
	}

	d_gfx.t_stack[d_gfx.t_stack_cnt] = d_gfx.transform;
	d_gfx.t_stack_cnt++;

}

void d_pop() {

	if (d_gfx.t_stack_cnt <= 0) {
		d_fail("transform stack underflow\n");
	}

	d_gfx.t_stack_cnt--;
	d_gfx.transform = d_gfx.t_stack[d_gfx.t_stack_cnt];

}

void d_move(vec3 p) {
	d_gfx.transform = mat4_mult(d_gfx.transform, mat4_translate(p));
}

void d_move_x(float x) {
	d_move(vec3f(x, 0.0, 0.0));
}

void d_move_y(float y) {
	d_move(vec3f(0.0, y, 0.0));
}

void d_move_z(float z) {
	d_move(vec3f(0.0, 0.0, z));
}

void d_move_xy(vec2 p) {
	d_move(vec3f(p.x, p.y, 0.0));
}

void d_scale(vec3 s) {
	d_gfx.transform = mat4_mult(d_gfx.transform, mat4_scale(s));
}

void d_scale_x(float x) {
	d_scale(vec3f(x, 1.0, 1.0));
}

void d_scale_y(float y) {
	d_scale(vec3f(1.0, y, 1.0));
}

void d_scale_z(float z) {
	d_scale(vec3f(1.0, 1.0, z));
}

void d_scale_xy(vec2 p) {
	d_scale(vec3f(p.x, p.y, 1.0));
}

void d_rot_x(float a) {
	d_gfx.transform = mat4_mult(d_gfx.transform, mat4_rot_x(a));
}

void d_rot_y(float a) {
	d_gfx.transform = mat4_mult(d_gfx.transform, mat4_rot_y(a));
}

void d_rot_z(float a) {
	d_gfx.transform = mat4_mult(d_gfx.transform, mat4_rot_z(a));
}

void d_use_cam(const d_cam *cam) {
	if (cam) {
		d_gfx.cur_cam = cam;
	} else {
		d_gfx.cur_cam = &d_gfx.default_cam;
	}
}

void d_use_shader(const d_shader *shader) {
	d_batch_flush(&d_gfx.batch);
	if (shader) {
		d_gfx.cur_shader = shader;
	} else {
		d_gfx.cur_shader = &d_gfx.default_shader;
	}
}

void d_use_font(const d_font *font) {
	if (font) {
		d_gfx.cur_font = font;
	} else {
		d_gfx.cur_font = &d_gfx.default_font;
	}
}

void d_use_canvas(const d_canvas *c) {
	d_batch_flush(&d_gfx.batch);
	d_gfx.cur_canvas = c;
	if (c) {
		d_gfx.default_cam.proj = mat4_ortho(c->width, c->height, NEAR, FAR);
		glViewport(0, 0, c->width, c->height);
	} else {
		d_gfx.default_cam.proj = mat4_ortho(d_width(), d_height(), NEAR, FAR);
		glViewport(0, 0, d_width(), d_height());
	}
}

vec2 d_origin_pt(d_origin o) {
	switch (o) {
		case D_TOP_LEFT: return vec2f(-0.5, 0.5);
		case D_TOP: return vec2f(0, 0.5);
		case D_TOP_RIGHT: return vec2f(0.5, 0.5);
		case D_LEFT: return vec2f(-0.5, 0);
		case D_CENTER: return vec2f(0, 0);
		case D_RIGHT: return vec2f(0.5, 0);
		case D_BOT_LEFT: return vec2f(-0.5, -0.5);
		case D_BOT: return vec2f(0, -0.5);
		case D_BOT_RIGHT: return vec2f(0.5, -0.5);
	}
	return vec2f(0.0, 0.0);
}

vec2 d_coord(d_origin o) {
	vec2 p = d_origin_pt(o);
	return vec2f(p.x * d_width(), p.y * d_height());
}

mat4 d_transform() {
	return d_gfx.transform;
}

vec2 d_mouse_pos_t() {
	return mat4_mult_vec2(mat4_invert(d_transform()), d_mouse_pos());
}

void d_draw(GLuint vbuf, GLuint ibuf, int count) {

	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glUseProgram(d_gfx.cur_shader->id);

	for (int i = 0; d_gfx.tex_slots[i] != NULL; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, d_gfx.tex_slots[i]->id);
	}

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 48, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 48, (void*)12);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 48, (void*)24);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 48, (void*)32);
	glEnableVertexAttribArray(3);

	d_send_mat4("u_model", d_gfx.transform);
	d_send_mat4("u_view", d_gfx.cur_cam->view);
	d_send_mat4("u_proj", d_gfx.cur_cam->proj);
	d_send_color("u_color", coloru());

	if (d_gfx.cur_canvas) {
		glBindFramebuffer(GL_FRAMEBUFFER, d_gfx.cur_canvas->fbuf);
	}

	glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	for (int i = 0; d_gfx.tex_slots[i] != NULL; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	glUseProgram(0);

}

// TODO
static void d_set_tex(const d_tex *t) {
	if (t) {
		if (t->id != d_gfx.tex_slots[0]->id) {
			d_batch_flush(&d_gfx.batch);
			d_gfx.tex_slots[0] = t;
		}
	} else {
		if (d_gfx.tex_slots[0]->id != d_gfx.default_tex.id) {
			d_batch_flush(&d_gfx.batch);
		}
		d_gfx.tex_slots[0] = &d_gfx.default_tex;
	}
}

void d_draw_raw(
	const d_vertex *verts,
	int vcount,
	const d_index *indices,
	int icount,
	const d_tex *tex
) {

	d_vertex tverts[vcount];
	memcpy(&tverts, verts, vcount * sizeof(d_vertex));

	for (int i = 0; i < vcount; i++) {
		tverts[i].pos = mat4_mult_vec3(d_gfx.transform, verts[i].pos);
	}

	d_set_tex(tex);
	d_batch_push(&d_gfx.batch, tverts, vcount, indices, icount);

}

void d_draw_mesh(const d_mesh *mesh) {
	d_batch_flush(&d_gfx.batch);
	d_draw(mesh->vbuf, mesh->ibuf, mesh->count);
}

void d_draw_tex(const d_tex *t, quad q, color c) {

	d_push();
	d_scale(vec3f(t->width * q.w, t->height * q.h, 1.0));

	vec2 uv0 = vec2f(q.x, 1.0 - q.y - q.h);
	vec2 uv1 = vec2f(q.x, 1.0 - q.y);
	vec2 uv2 = vec2f(q.x + q.w, 1.0 - q.y);
	vec2 uv3 = vec2f(q.x + q.w, 1.0 - q.y - q.h);

	d_vertex verts[] = {
		{
			.pos = vec3f(-0.5, -0.5, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = uv0,
			.color = c
		},
		{
			.pos = vec3f(-0.5, 0.5, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = uv1,
			.color = c
		},
		{
			.pos = vec3f(0.5, 0.5, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = uv2,
			.color = c
		},
		{
			.pos = vec3f(0.5, -0.5, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = uv3,
			.color = c
		},
	};

	d_index indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	d_draw_raw(verts, 4, indices, 6, t);

	d_pop();

}

void d_draw_ftext(const d_ftext *ftext) {

	for (int i = 0; i < ftext->len; i++) {
		d_fchar ch = ftext->chars[i];
		d_push();
		d_move_xy(ch.pos);
		d_scale_xy(vec2f(ftext->scale, ftext->scale));
		d_draw_tex(ftext->tex, ch.quad, ch.color);
		d_pop();
	}

}

void d_draw_text(const char *text, float size, float width, d_origin orig, color c) {
	d_ftext ftext = d_fmt_text(text, size, width, orig, c);
	d_draw_ftext(&ftext);
}

void d_draw_canvas(const d_canvas *canvas, color c) {
	d_draw_tex(&canvas->ctex, quadu(), c);
}

void d_draw_rect(vec2 p1, vec2 p2, color c) {

	vec2 pp1 = vec2_min(p1, p2);
	vec2 pp2 = vec2_max(p1, p2);

	d_vertex verts[] = {
		{
			.pos = vec3f(pp1.x, pp1.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 0.0),
			.color = c
		},
		{
			.pos = vec3f(pp1.x, pp2.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 1.0),
			.color = c
		},
		{
			.pos = vec3f(pp2.x, pp2.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(1.0, 1.0),
			.color = c
		},
		{
			.pos = vec3f(pp2.x, pp1.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(1.0, 0.0),
			.color = c
		},
	};

	d_index indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	d_draw_raw(verts, 4, indices, 6, NULL);

}

void d_draw_line(vec2 p1, vec2 p2, float width, color c) {

	vec2 pp1 = vec2_min(p1, p2);
	vec2 pp2 = vec2_max(p1, p2);
	vec2 dpos1 = vec2_scale(vec2_unit(vec2_normal(vec2_sub(pp2, pp1))), width / 2.0);
	vec2 dpos2 = vec2_scale(vec2_unit(vec2_normal(vec2_sub(pp1, pp2))), width / 2.0);
	vec2 cp1 = vec2_sub(pp1, dpos1);
	vec2 cp2 = vec2_add(pp1, dpos1);
	vec2 cp3 = vec2_sub(pp2, dpos2);
	vec2 cp4 = vec2_add(pp2, dpos2);

	d_vertex verts[] = {
		{
			.pos = vec3f(cp1.x, cp1.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 0.0),
			.color = c
		},
		{
			.pos = vec3f(cp2.x, cp2.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 0.0),
			.color = c
		},
		{
			.pos = vec3f(cp3.x, cp3.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 0.0),
			.color = c
		},
		{
			.pos = vec3f(cp4.x, cp4.y, 0.0),
			.normal = vec3f(0.0, 0.0, 1.0),
			.uv = vec2f(0.0, 0.0),
			.color = c
		},
	};

	d_index indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	d_draw_raw(verts, 4, indices, 6, NULL);

}

void d_draw_lrect(vec2 p1, vec2 p3, float w, color c) {
	vec2 p2 = vec2f(p1.x, p3.y);
	vec2 p4 = vec2f(p3.x, p1.y);
	d_draw_line(p1, p2, w, c);
	d_draw_line(p2, p3, w, c);
	d_draw_line(p3, p4, w, c);
	d_draw_line(p4, p1, w, c);
}

// TODO
void d_draw_circle(vec2 p, float r, color c) {
	// ...
}

// TODO: support \n
d_ftext d_fmt_text(const char *text, float size, float width, d_origin orig, color c) {

	d_ftext ftext = {0};

	int len = strlen(text);
	const d_tex *tex = &d_gfx.cur_font->tex;
	float qw = d_gfx.cur_font->qw;
	float qh = d_gfx.cur_font->qh;
	float scale = size / (qh * tex->height);
	float gw = qw * tex->width * scale;
	float gh = size;
	int alen = 0;

	vec2 offset = d_origin_pt(orig);

	float lw = 0.0;
	float y = 0.0;
	int last_space = -1;
	int last_i = 0;
	int ii = 0;

	for (int i = 0; i < len; i++) {

		char ch = text[i];

		if (!isprint(ch)) {
			continue;
		}

		if (ch == ' ') {
			last_space = i;
		}

		lw += gw;
		alen++;

		bool overflow = (lw + gw) > width;
		bool last = i == len - 1;

		if (overflow || last) {

			int to = last ? i : (last_space >= 0 ? last_space : i);
			float ox = 0.5 * gw - (to - last_i + 1) * gw * (offset.x + 0.5);
			float x = 0.0;

			for (int j = last_i; j <= to; j++) {

				char ch = text[j];

				if (!isprint(ch)) {
					continue;
				}

				vec2 qpos = d_gfx.cur_font->map[(int)ch];
				quad q = quadf(qpos.x, qpos.y, qw, qh);

				ftext.chars[ii++] = (d_fchar) {
					.pos = vec2f(x + ox, -y),
					.color = c,
					.quad = q,
				};

				x += gw;

			}

			lw = 0.0;
			last_i = to + 1;
			i = to;
			y += gh;
			last_space = -1;

		}

	}

	ftext.len = alen;
	ftext.width = width;
	ftext.height = y;
	ftext.tex = tex;
	ftext.scale = scale;

	float oy = -0.5 * gh - (offset.y - 0.5) * y;

	for (int i = 0; i < ftext.len; i++) {
		ftext.chars[i].pos.y += oy;
	}

	return ftext;

}

