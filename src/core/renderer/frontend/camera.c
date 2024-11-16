#include <camera.h>
#include <math_utils.h>

void camera_get_view_matrix(camera_t *camera, mat4f_t *mat)
{
    mat4_look_at(camera->position, vec3_add(camera->front,camera->position), camera->up, mat);
}

void camera_get_projection(camera_t *camera, mat4f_t *mat)
{
    mat4_perspective(camera->fov / 2.0f, camera->aspect, camera->znear, camera->zfar, mat);
}

