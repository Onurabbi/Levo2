#ifndef MATH_H_
#define MATH_H_

#include "math_types.h"
#include <stdbool.h>

void    transform_from_TRS(mat4f_t *transform, vec3f_t translation, quat_t rotation, vec3f_t scale);

void    mat4_look_at(vec3f_t from, vec3f_t to, vec3f_t up, mat4f_t *mat);
void    mat4_orthographic(float bottom, float top, float left, float right, float near, float far, mat4f_t *m);
void    mat4_perspective(float theta, float aspect, float near, float far, mat4f_t *mat);
void    mat4_rotate(const mat4f_t *in, mat4f_t *out, vec3f_t axis, float theta);
void    mat4_rotate_w_quat(const mat4f_t *in, mat4f_t *out, quat_t quat);
void    mat4_translate(const mat4f_t *in, mat4f_t *out, float x, float y, float z);
void    mat4_scale(const mat4f_t *in, mat4f_t *out, float x, float y, float z);
void    mat4_multiply(const mat4f_t *a, const mat4f_t *b, mat4f_t *out);
void    mat4_inverse(const mat4f_t *a, mat4f_t *out);
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

void quat_to_rot_mat4(quat_t quat, mat4f_t *rot);

#endif

