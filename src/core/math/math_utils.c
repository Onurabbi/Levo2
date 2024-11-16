#include "math_utils.h"

#include <logger.h>

#include <stdint.h>
#include <assert.h>
#include <math.h>

void mat4_multiply(const mat4f_t *a, const mat4f_t *b, mat4f_t *out)
{
    out->m[0][0] = a->m[0][0] * b->m[0][0] + a->m[0][1] * b->m[1][0] + a->m[0][2] * b->m[2][0] + a->m[0][3] * b->m[3][0]; 
    out->m[0][1] = a->m[0][0] * b->m[0][1] + a->m[0][1] * b->m[1][1] + a->m[0][2] * b->m[2][1] + a->m[0][3] * b->m[3][1]; 
    out->m[0][2] = a->m[0][0] * b->m[0][2] + a->m[0][1] * b->m[1][2] + a->m[0][2] * b->m[2][2] + a->m[0][3] * b->m[3][2]; 
    out->m[0][3] = a->m[0][0] * b->m[0][3] + a->m[0][1] * b->m[1][3] + a->m[0][2] * b->m[2][3] + a->m[0][3] * b->m[3][3]; 
    out->m[1][0] = a->m[1][0] * b->m[0][0] + a->m[1][1] * b->m[1][0] + a->m[1][2] * b->m[2][0] + a->m[1][3] * b->m[3][0]; 
    out->m[1][1] = a->m[1][0] * b->m[0][1] + a->m[1][1] * b->m[1][1] + a->m[1][2] * b->m[2][1] + a->m[1][3] * b->m[3][1]; 
    out->m[1][2] = a->m[1][0] * b->m[0][2] + a->m[1][1] * b->m[1][2] + a->m[1][2] * b->m[2][2] + a->m[1][3] * b->m[3][2]; 
    out->m[1][3] = a->m[1][0] * b->m[0][3] + a->m[1][1] * b->m[1][3] + a->m[1][2] * b->m[2][3] + a->m[1][3] * b->m[3][3]; 
    out->m[2][0] = a->m[2][0] * b->m[0][0] + a->m[2][1] * b->m[1][0] + a->m[2][2] * b->m[2][0] + a->m[2][3] * b->m[3][0]; 
    out->m[2][1] = a->m[2][0] * b->m[0][1] + a->m[2][1] * b->m[1][1] + a->m[2][2] * b->m[2][1] + a->m[2][3] * b->m[3][1]; 
    out->m[2][2] = a->m[2][0] * b->m[0][2] + a->m[2][1] * b->m[1][2] + a->m[2][2] * b->m[2][2] + a->m[2][3] * b->m[3][2]; 
    out->m[2][3] = a->m[2][0] * b->m[0][3] + a->m[2][1] * b->m[1][3] + a->m[2][2] * b->m[2][3] + a->m[2][3] * b->m[3][3]; 
    out->m[3][0] = a->m[3][0] * b->m[0][0] + a->m[3][1] * b->m[1][0] + a->m[3][2] * b->m[2][0] + a->m[3][3] * b->m[3][0]; 
    out->m[3][1] = a->m[3][0] * b->m[0][1] + a->m[3][1] * b->m[1][1] + a->m[3][2] * b->m[2][1] + a->m[3][3] * b->m[3][1]; 
    out->m[3][2] = a->m[3][0] * b->m[0][2] + a->m[3][1] * b->m[1][2] + a->m[3][2] * b->m[2][2] + a->m[3][3] * b->m[3][2]; 
    out->m[3][3] = a->m[3][0] * b->m[0][3] + a->m[3][1] * b->m[1][3] + a->m[3][2] * b->m[2][3] + a->m[3][3] * b->m[3][3]; 
}

vec3f_t vec3_normalize(vec3f_t in)
{
    vec3f_t result = {0};
    float len = sqrtf(in.x * in.x + in.y * in.y + in.z * in.z);

    float epsilon = nextafterf(0, INFINITY);
    assert(len > epsilon);

    result.x = in.x / len;
    result.y = in.y / len;
    result.z = in.z / len;
    return result;
}

mat4f_t mat4_translate(float x, float y, float z)
{
    mat4f_t trans = {0};
    trans.m[0][0] = 1.0f;
    trans.m[1][1] = 1.0f;
    trans.m[2][2] = 1.0f;
    trans.m[3][3] = 1.0f;
    trans.m[3][0] = x;
    trans.m[3][1] = y;
    trans.m[3][2] = z;

    return trans;
}

/*
    From: https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
*/
mat4f_t mat4_rotate(vec3f_t axis, float theta)
{
    axis = vec3_normalize(axis);

    mat4f_t rot = {0};
    rot.m[0][0] = cosf(theta) + axis.x * axis.x * (1 - cosf(theta));
    rot.m[0][1] = axis.x * axis.y * (1 - cosf(theta)) - axis.z * sinf(theta);
    rot.m[0][2] = axis.x * axis.z * (1 - cosf(theta)) + axis.y * sinf(theta);

    rot.m[1][0] = axis.y * axis.x * (1 - cosf(theta)) + axis.z * sinf(theta);
    rot.m[1][1] = cosf(theta)  + axis.y * axis.y * (1 - cosf(theta));
    rot.m[1][2] = axis.y * axis.z * (1 - cosf(theta)) - axis.x * sinf(theta);

    rot.m[2][0] = axis.z * axis.x * (1 - cosf(theta)) - axis.y * sinf(theta);
    rot.m[2][1] = axis.z * axis.y * (1 - cosf(theta)) + axis.x * sinf(theta);
    rot.m[2][2] = cosf(theta) + axis.z * axis.z * (1 - cosf(theta));

    rot.m[3][3] = 1.0f;

    return rot;
}

mat4f_t mat4_rotate_w_quat(quat_t quat)
{
    mat4f_t rot = {0};
    quat_to_rot_mat4(quat, &rot);
    return rot;
}

mat4f_t mat4_identity(void)
{
    mat4f_t result = {0};
    result.m[0][0] = 1.0f;
    result.m[1][1] = 1.0f;
    result.m[2][2] = 1.0f;
    result.m[3][3] = 1.0f;
    return result;
}

mat4f_t mat4_scale(float x, float y, float z)
{
   mat4f_t scale = mat4_identity();

   scale.m[0][0] = x;
   scale.m[1][1] = y;
   scale.m[2][2] = z; 

   return scale;
}

vec3f_t vec3_subtract(vec3f_t a, vec3f_t b)
{
    return (vec3f_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3f_t vec3_add(vec3f_t a, vec3f_t b)
{
    return (vec3f_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

float vec3_dot(vec3f_t a, vec3f_t b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z);
}

vec3f_t vec3_cross(vec3f_t a, vec3f_t b)
{
    vec3f_t result = {0};
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

void mat4_look_at(vec3f_t from, vec3f_t to, vec3f_t arbitrary_up, mat4f_t *mat)
{
    vec3f_t forward = vec3_normalize(vec3_subtract(from, to));
    vec3f_t right = vec3_normalize(vec3_cross(arbitrary_up, forward));
    vec3f_t up = vec3_cross(forward, right);

    mat->m[0][0] = right.x;
    mat->m[1][0] = right.y;
    mat->m[2][0] = right.z;
    mat->m[3][0] = -vec3_dot(right, from);

    mat->m[0][1] = up.x;
    mat->m[1][1] = up.y;
    mat->m[2][1] = up.z;
    mat->m[3][1] = -vec3_dot(up, from);

    mat->m[0][2] = forward.x;
    mat->m[1][2] = forward.y;
    mat->m[2][2] = forward.z;
    mat->m[3][2] = -vec3_dot(forward, from);

    mat->m[0][3] = 0;
    mat->m[1][3] = 0;
    mat->m[2][3] = 0;
    mat->m[3][3] = 1.0f;
}

void mat4_orthographic(float bottom, float top, float left, float right, float near, float far, mat4f_t *m)
{
    m->m[0][0] = 2 / (right - left);
    m->m[0][1] = 0;
    m->m[0][2] = 0;
    m->m[0][3] = 0;

    m->m[1][0] = 0;
    m->m[1][1] = 2 / (top - bottom);
    m->m[1][2] = 0;
    m->m[1][3] = 0;

    m->m[2][0] = 0;
    m->m[2][1] = 0;
    m->m[2][2] = -2 / (far - near);
    m->m[2][3] = 0;

    m->m[3][0] = -(right + left) / (right - left);
    m->m[3][1] = -(top + bottom) / (top - bottom);
    m->m[3][2] = -(far + near) / (far - near);
    m->m[3][3] = 1;
}

void mat4_perspective(float half_theta, float aspect, float near, float far, mat4f_t *mat)
{
    memset(mat, 0, sizeof(*mat));

    float tan_half_theta = tanf(half_theta);
    
    mat->m[0][0] = 1.0f / (aspect * tan_half_theta);
    mat->m[1][1] = 1.0f / (tan_half_theta);
    mat->m[2][2] = far / (near - far);
    mat->m[2][3] = -1.0f;
    mat->m[3][2] = -(far * near)/(far - near);
}

void mat4_transpose(const mat4f_t *mat, mat4f_t *out)
{
    out->m[0][0] = mat->m[0][0];
    out->m[0][1] = mat->m[1][0];
    out->m[0][2] = mat->m[2][0];
    out->m[0][3] = mat->m[3][0];

    out->m[1][0] = mat->m[0][1];
    out->m[1][1] = mat->m[1][1];
    out->m[1][2] = mat->m[2][1];
    out->m[1][3] = mat->m[3][1];

    out->m[2][0] = mat->m[0][2];
    out->m[2][1] = mat->m[1][2];
    out->m[2][2] = mat->m[2][2];
    out->m[2][3] = mat->m[3][2];

    out->m[3][0] = mat->m[0][3];
    out->m[3][1] = mat->m[1][3];
    out->m[3][2] = mat->m[2][3];
    out->m[3][3] = mat->m[3][3];
}

void mat4_inverse(const mat4f_t *mat, mat4f_t *out)
{
    float t0 = mat->m[2][2] * mat->m[3][3];
    float t1 = mat->m[3][2] * mat->m[2][3];
    float t2 = mat->m[1][2] * mat->m[3][3];
    float t3 = mat->m[3][2] * mat->m[1][3];
    float t4 = mat->m[1][2] * mat->m[2][3];
    float t5 = mat->m[2][2] * mat->m[1][3];
    float t6 = mat->m[0][2] * mat->m[3][3];
    float t7 = mat->m[3][2] * mat->m[0][3];
    float t8 = mat->m[0][2] * mat->m[2][3];
    float t9 = mat->m[2][2] * mat->m[0][3];
    float t10 = mat->m[0][2] * mat->m[1][3];
    float t11 = mat->m[1][2] * mat->m[0][3];
    float t12 = mat->m[2][0] * mat->m[3][1];
    float t13 = mat->m[3][0] * mat->m[2][1];
    float t14 = mat->m[1][0] * mat->m[3][1];
    float t15 = mat->m[3][0] * mat->m[1][1];
    float t16 = mat->m[1][0] * mat->m[2][1];
    float t17 = mat->m[2][0] * mat->m[1][1];
    float t18 = mat->m[0][0] * mat->m[3][1];
    float t19 = mat->m[3][0] * mat->m[0][1];
    float t20 = mat->m[0][0] * mat->m[2][1];
    float t21 = mat->m[2][0] * mat->m[0][1];
    float t22 = mat->m[0][0] * mat->m[1][1];
    float t23 = mat->m[1][0] * mat->m[0][1];

    out->m[0][0] = (t0 * mat->m[1][1] + t3 * mat->m[2][1] + t4 * mat->m[3][1]) - (t1 * mat->m[1][1] + t2 * mat->m[2][1] + t5 * mat->m[3][1]);
    out->m[0][1] = (t1 * mat->m[0][1] + t6 * mat->m[2][1] + t9 * mat->m[3][1]) - (t0 * mat->m[0][1] + t7 * mat->m[2][1] + t8 * mat->m[3][1]);
    out->m[0][2] = (t2 * mat->m[0][1] + t7 * mat->m[1][1] + t10 * mat->m[3][1]) - (t3 * mat->m[0][1] + t6 * mat->m[1][1] + t11 * mat->m[3][1]);
    out->m[0][3] = (t5 * mat->m[0][1] + t8 * mat->m[1][1] + t11 * mat->m[2][1]) - (t4 * mat->m[0][1] + t9 * mat->m[1][1] + t10 * mat->m[2][1]);

    float d = 1.0f / (mat->m[0][0] * out->m[0][0] + mat->m[1][0] * out->m[0][1] + mat->m[2][0] * out->m[0][2] + mat->m[3][0] * out->m[0][3]);

    out->m[0][0] *= d;
    out->m[0][1] *= d;
    out->m[0][2] *= d;
    out->m[0][3] *= d;

    out->m[1][0] = d * ((t1 * mat->m[1][0] + t2 * mat->m[2][0] + t5 * mat->m[3][0]) - (t0 * mat->m[1][0] + t3 * mat->m[2][0] + t4 * mat->m[3][0]));
    out->m[1][1] = d * ((t0 * mat->m[0][0] + t7 * mat->m[2][0] + t8 * mat->m[3][0]) - (t1 * mat->m[0][0] + t6 * mat->m[2][0] + t9 * mat->m[3][0]));
    out->m[1][2] = d * ((t3 * mat->m[0][0] + t6 * mat->m[1][0] + t11 * mat->m[3][0]) - (t2 * mat->m[0][0] + t7 * mat->m[1][0] + t10 * mat->m[3][0]));
    out->m[1][3] = d * ((t4 * mat->m[0][0] + t9 * mat->m[1][0] + t10 * mat->m[2][0]) - (t5 * mat->m[0][0] + t8 * mat->m[1][0] + t11 * mat->m[2][0]));

    out->m[2][0] = d * ((t12 * mat->m[1][3] + t15 * mat->m[2][3] + t16 * mat->m[3][3]) - (t13 * mat->m[1][3] + t14 * mat->m[2][3] + t17 * mat->m[3][3]));
    out->m[2][1] = d * ((t13 * mat->m[0][3] + t18 * mat->m[2][3] + t21 * mat->m[3][3]) - (t12 * mat->m[0][3] + t19 * mat->m[2][3] + t20 * mat->m[3][3]));
    out->m[2][2] = d * ((t14 * mat->m[0][3] + t19 * mat->m[1][3] + t22 * mat->m[3][3]) - (t15 * mat->m[0][3] + t18 * mat->m[1][3] + t23 * mat->m[3][3]));
    out->m[2][3] = d * ((t17 * mat->m[0][3] + t20 * mat->m[1][3] + t23 * mat->m[2][3]) - (t16 * mat->m[0][3] + t21 * mat->m[1][3] + t22 * mat->m[2][3]));

    out->m[3][0] = d * ((t14 * mat->m[2][2] + t17 * mat->m[3][2] + t13 * mat->m[1][2]) - (t16 * mat->m[3][2] + t12 * mat->m[1][2] + t15 * mat->m[2][2]));
    out->m[3][1] = d * ((t20 * mat->m[3][2] + t12 * mat->m[0][2] + t19 * mat->m[2][2]) - (t18 * mat->m[2][2] + t21 * mat->m[3][2] + t13 * mat->m[0][2]));
    out->m[3][2] = d * ((t18 * mat->m[1][2] + t23 * mat->m[3][2] + t15 * mat->m[0][2]) - (t22 * mat->m[3][2] + t14 * mat->m[0][2] + t19 * mat->m[1][2]));
    out->m[3][3] = d * ((t22 * mat->m[2][2] + t16 * mat->m[0][2] + t21 * mat->m[1][2]) - (t20 * mat->m[1][2] + t23 * mat->m[2][2] + t17 * mat->m[0][2]));
}

vec2f_t vec2_normalize(vec2f_t in)
{
    vec2f_t result = {0,0};
    float len = sqrtf(in.x * in.x + in.y * in.y);
    if (len > nextafterf(0, INFINITY)) {
        result.x = in.x / len;
        result.y = in.y / len;
    }
    return result;
}

vec2f_t vec2_divide(vec2f_t v1, vec2f_t v2)
{
    return (vec2f_t){v1.x / v2.x, v1.y / v2.y}; 
}

vec2f_t vec2_subtract(vec2f_t a, vec2f_t b)
{
    return (vec2f_t){a.x - b.x, a.y - b.y}; 
}

vec2f_t vec2_multiply(vec2f_t a, vec2f_t b)
{
    return (vec2f_t){a.x * b.x, a.y * b.y}; 
}

vec2f_t vec2_add(vec2f_t a, vec2f_t b)
{
    return (vec2f_t){a.x + b.x, a.y + b.y}; 
}


void quat_to_rot_mat4(quat_t quat, mat4f_t *rot)
{
    rot->m[0][0] = 1.0f - 2 * (quat.y * quat.y + quat.z * quat.z) ;
    rot->m[0][1] = 2.0f * (quat.x * quat.y + quat.w * quat.z);
    rot->m[0][2] = 2.0f * (quat.x * quat.z - quat.w * quat.y);
    rot->m[0][3] = 0.0f;

    rot->m[1][0] = 2.0f * (quat.x * quat.y - quat.w * quat.z);
    rot->m[1][1] = 1.0f - 2.0f * (quat.z * quat.z + quat.x * quat.x);
    rot->m[1][2] = 2.0f * (quat.y * quat.z + quat.w * quat.x);
    rot->m[1][3] = 0;

    rot->m[2][0] = 2.0f * (quat.x * quat.z + quat.w * quat.y);
    rot->m[2][1] = 2.0f * (quat.y * quat.z - quat.w * quat.x);
    rot->m[2][2] = 1.0f - 2.0f * (quat.x * quat.x + quat.y * quat.y);
    rot->m[2][3] = 0;

    rot->m[3][0] = 0;
    rot->m[3][1] = 0;
    rot->m[3][2] = 0;
    rot->m[3][3] = 1.0f;
}

void transform_from_TRS(mat4f_t *transform, vec3f_t translation, quat_t rotation, vec3f_t scale)
{
    mat4f_t scale_mat = mat4_scale(scale.x, scale.y, scale.z);
    mat4f_t rot_mat = mat4_rotate_w_quat(rotation);
    mat4f_t trans_mat = mat4_translate(translation.x, translation.y, translation.z);

    mat4f_t scaled = {0};
    mat4f_t rotated = {0};

    mat4f_t temp = *transform;
    
    mat4_multiply(&temp, &scale_mat, &scaled);
    mat4_multiply(&scaled, &rot_mat, &rotated);
    mat4_multiply(&rotated, &trans_mat, transform);
}

vec3f_t vec3_lerp(vec3f_t a, vec3f_t b, float t)
{
    vec3f_t result = {0};
    result.x = a.x * (1.0f - t) + b.x * t;
    result.y = a.y * (1.0f - t) + b.y * t;
    result.z = a.z * (1.0f - t) + b.z * t;
    return result; 
}

vec4f_t vec4_normalize(vec4f_t in)
{
    vec4f_t result = {0};
    float len = sqrtf(in.x * in.x + in.y * in.y + in.z * in.z + in.w * in.w);

    float epsilon = nextafterf(0, INFINITY);
    assert(len > epsilon);

    result.x = in.x / len;
    result.y = in.y / len;
    result.z = in.z / len;
    result.w = in.w / len;
    return result;
}

vec4f_t vec4_lerp(vec4f_t a, vec4f_t b, float t)
{
    vec4f_t result = {0};
    result.x = a.x * (1.0f - t) + b.x * t;
    result.y = a.y * (1.0f - t) + b.y * t;
    result.z = a.z * (1.0f - t) + b.z * t;
    result.w = a.w * (1.0f - t) + b.w * t;
    return result;
}

static float quat_dot(quat_t a, quat_t b)
{
    return (a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z);
}

quat_t quat_normalize(quat_t in)
{
    quat_t result = {0};
    float len = sqrtf(in.x * in.x + in.y * in.y + in.z * in.z + in.w * in.w);

    float epsilon = nextafterf(0, INFINITY);
    if (len < epsilon) {
        return result;
    }

    result.x = in.x / len;
    result.y = in.y / len;
    result.z = in.z / len;
    result.w = in.w / len;

    return result;
}

quat_t quat_lerp(quat_t a, quat_t b, float t)
{
    quat_t result = {0};
    result.w = a.w * (1.0f - t) + b.w * t;
    result.x = a.x * (1.0f - t) + b.x * t;
    result.y = a.y * (1.0f - t) + b.y * t;
    result.z = a.z * (1.0f - t) + b.z * t;
    return result;
}

quat_t quat_slerp(quat_t a, quat_t b, float t)
{
    quat_t result = {0};

    quat_t z = b;

    float cos_theta = quat_dot(a,b);
    
    //if dot is negative, the slerp will take the long way around the sphere
    //negate one to fix this
    if (cos_theta < 0) {
        z.x *= -1.0f;
        z.y *= -1.0f;
        z.z *= -1.0f;
        z.w *= -1.0f;

        cos_theta = -cos_theta;
    }

    //perform a linear interpolation when cos_theta is close to one to avoid side effect of sin(angle) becoming a zero denominator
    if (cos_theta > nextafterf(1.0f, -INFINITY)) {
        result = quat_lerp(a,z,t);
    } else {
        float angle = acosf(cos_theta);
        result.w = a.w * sinf((1.0f - t) * angle) + z.w * sinf(t * angle) / sinf(angle);
        result.x = a.x * sinf((1.0f - t) * angle) + z.x * sinf(t * angle) / sinf(angle);
        result.y = a.y * sinf((1.0f - t) * angle) + z.y * sinf(t * angle) / sinf(angle);
        result.z = a.z * sinf((1.0f - t) * angle) + z.z * sinf(t * angle) / sinf(angle);
    }
    return result;
}

