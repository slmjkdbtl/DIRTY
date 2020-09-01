// wengwengweng

#ifndef D_MATH_H
#define D_MATH_H

typedef struct {
	float x;
	float y;
} vec2;

vec2 vec2_add(vec2, vec2);
vec2 vec2_sub(vec2, vec2);
vec2 vec2_scale(vec2, float);
void vec2_print(vec2);

typedef struct {
	float x;
	float y;
	float z;
} vec3;

vec3 make_vec3(float, float, float);
vec3 vec3_add(vec3, vec3);
vec3 vec3_sub(vec3, vec3);
vec3 vec3_scale(vec3, float);
float vec3_dist(vec3, vec3);
float vec3_len(vec3);
vec3 vec3_unit(vec3);
vec3 vec3_cross(vec3, vec3);
float vec3_dot(vec3, vec3);
void vec3_print(vec3);

typedef struct {
	float r;
	float g;
	float b;
	float a;
} color;

typedef struct {
	float m[16];
} mat4;

mat4 make_mat4();
mat4 mat4_mult(mat4, mat4);
mat4 mat4_scale(vec3);
mat4 mat4_translate(vec3);
mat4 mat4_ortho(float, float, float, float);
mat4 mat4_proj(float, float, float, float);
mat4 mat4_view(vec3, vec3, vec3);
mat4 mat4_rot_x(float);
mat4 mat4_rot_y(float);
mat4 mat4_rot_z(float);
void mat4_print(mat4);

#endif
