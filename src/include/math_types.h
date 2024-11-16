#ifndef MATH_TYPES_H_
#define MATH_TYPES_H_

#define PI 3.14159265358979323846
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#include <stdint.h>

typedef struct 
{
    int32_t x,y;
}vec2i_t;

typedef struct 
{
    int32_t x,y,z;
}vec3i_t;

typedef struct 
{
    int32_t x,y,z,w;
}vec4i_t;

typedef struct 
{
    float x,y;
}vec2f_t;

typedef struct 
{
    float x,y,z;
}vec3f_t;

typedef struct
{
    float x,y,z,w;
}vec4f_t;

typedef struct
{
    float m[4][4];
}mat4f_t;

typedef struct 
{
    vec2f_t min;
    vec2f_t size;
}rect_t;

typedef struct
{
    float w,x,y,z;
}quat_t;

#endif

