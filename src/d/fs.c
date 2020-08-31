// wengwengweng

#include <stdbool.h>
#include <unistd.h>
#include <SDL2/SDL.h>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#endif

const char* d_fread(const char* path) {

	int c;
	FILE* file = fopen(path, "r");

	if (!file) {
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = malloc(size);
	fread(buffer, 1, size, file);

	fclose(file);

	return buffer;

}

bool d_fexists(const char* path) {
	return access(path, F_OK) != -1;
}

static const char* res_path() {
#ifdef __APPLE__
	NSBundle* bundle = [ NSBundle mainBundle ];
	NSString* path = [ bundle resourcePath ];
	return [ path UTF8String ];
#else
	return "";
#endif
}

