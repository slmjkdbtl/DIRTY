// wengwengweng

#ifndef LDIRTY_UTILS_H
#define LDIRTY_UTILS_H

#include <stdbool.h>
#include <string.h>

#define lua_pushudata(L, T, V) \
	T* lv = lua_newuserdata(L, sizeof(T)); \
	luaL_setmetatable(L, #T); \
	memcpy(lv, V, sizeof(T)); \

typedef struct {
	const char *str;
	int val;
} enum_map;

bool luaL_checkboolean(lua_State *L, int pos);
void *luaL_optudata(lua_State *L, int pos, const char *type, void *def);
void lua_import(lua_State *L, luaL_Reg *reg);
bool streq(const char *a, const char *b);
int str_to_enum(const char *str, enum_map *map);
bool is_file(const char *path);
bool is_dir(const char *path);
void to_dir(char *path);
void *get_res_dir(char *buf);

#endif

