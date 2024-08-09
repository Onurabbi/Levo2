#ifndef MATH_H_
#define MATH_H_

#include "math_types.h"
#include <stdbool.h>

void    look_at(vec3f_t from, vec3f_t to, vec3f_t up, mat4f_t *mat);
void    ortho(float bottom, float top, float left, float right, float near, float far, mat4f_t *m);
void    perspective(float theta, float aspect, float near, float far, mat4f_t *mat);
void    mat4_rotate(const mat4f_t *in, mat4f_t *out, vec3f_t axis, float theta);
void    mat4_translate(const mat4f_t *in, mat4f_t *out, float x, float y, float z);
mat4f_t mat4_identity(void);

vec3f_t vec3_normalize(vec3f_t in);
vec3f_t vec3_subtract(vec3f_t a, vec3f_t b);
vec3f_t vec3_add(vec3f_t a, vec3f_t b);
float   vec3_dot(vec3f_t a, vec3f_t b);
vec3f_t vec3_cross(vec3f_t a, vec3f_t b);

vec2f_t vec2_normalize(vec2f_t in);
vec2f_t vec2_divide(vec2f_t a, vec2f_t b);
vec2f_t vec2_subtract(vec2f_t a, vec2f_t b);
vec2f_t vec2_multiply(vec2f_t a, vec2f_t b);
vec2f_t vec2_add(vec2f_t a, vec2f_t b);
#endif

