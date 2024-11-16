#ifndef CAMERA_H
#define CAMERA_H

#include <renderer_types.h>
#include <math_types.h>

void camera_get_view_matrix(camera_t *camera, mat4f_t *mat);
void camera_get_projection(camera_t *camera, mat4f_t *mat);
#endif
