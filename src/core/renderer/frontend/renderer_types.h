#ifndef RENDERER_TYPES_H_
#define RENDERER_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "../../platform/platform.h"

#define MAX_SSBO_PER_SKINNED_MODEL       32
#define MAX_TEXTURES_PER_SKINNED_MODEL   32

typedef enum
{
    SHADER_TYPE_STATIC_GEOMETRY,
    SHADER_TYPE_SKINNED_GEOMETRY,
}renderer_shader_type_e;

typedef enum 
{
    /** @brief Keeps the current value. */
    RENDERER_STENCIL_OP_KEEP = 0,
    /** @brief Sets the stencil buffer value to 0. */
    RENDERER_STENCIL_OP_ZERO = 1,
    /** @brief Sets the stencil buffer value to _ref_, as specified in the function. */
    RENDERER_STENCIL_OP_REPLACE = 2,
    /** @brief Increments the current stencil buffer value. Clamps to the maximum representable unsigned value. */
    RENDERER_STENCIL_OP_INCREMENT_AND_CLAMP = 3,
    /** @brief Decrements the current stencil buffer value. Clamps to 0. */
    RENDERER_STENCIL_OP_DECREMENT_AND_CLAMP = 4,
    /** @brief Bitwise inverts the current stencil buffer value. */
    RENDERER_STENCIL_OP_INVERT = 5,
    /** @brief Increments the current stencil buffer value. Wraps stencil buffer value to zero when incrementing the maximum representable unsigned value. */
    RENDERER_STENCIL_OP_INCREMENT_AND_WRAP = 6,
    /** @brief Decrements the current stencil buffer value. Wraps stencil buffer value to the maximum representable unsigned value when decrementing a stencil buffer value of zero. */
    RENDERER_STENCIL_OP_DECREMENT_AND_WRAP = 7
} renderer_stencil_op_e;

typedef enum
{
    /** @brief Specifies that the comparison always evaluates false. */
    RENDERER_COMPARE_OP_NEVER = 0,
    /** @brief Specifies that the comparison evaluates reference < test. */
    RENDERER_COMPARE_OP_LESS = 1,
    /** @brief Specifies that the comparison evaluates reference = test. */
    RENDERER_COMPARE_OP_EQUAL = 2,
    /** @brief Specifies that the comparison evaluates reference <= test. */
    RENDERER_COMPARE_OP_LESS_OR_EQUAL = 3,
    /** @brief Specifies that the comparison evaluates reference > test. */
    RENDERER_COMPARE_OP_GREATER = 4,
    /** @brief Specifies that the comparison evaluates reference != test.*/
    RENDERER_COMPARE_OP_NOT_EQUAL = 5,
    /** @brief Specifies that the comparison evaluates reference >= test. */
    RENDERER_COMPARE_OP_GREATER_OR_EQUAL = 6,
    /** @brief Specifies that the comparison is always true. */
    RENDERER_COMPARE_OP_ALWAYS = 7
} renderer_compare_op_e;

typedef enum
{
    RENDERER_WINDING_CCW = 0,
    RENDERER_WINDING_CW = 1
}renderer_winding_e;

typedef enum 
{
    RENDERER_CONFIG_FLAG_VSYNC_ENABLED_BIT = 0x1,
    RENDERER_CONFIG_FLAG_POWER_SAVING_BIT = 0x2,
    RENDERER_CONFIG_FLAG_ENABLE_VALIDATION = 0x4,
}renderer_config_flag_bit_e;

typedef enum
{
    RENDER_DATA_SCENE_UNIFORMS,
    RENDER_DATA_SKINNED_MODEL,
    RENDER_DATA_STATIC_MODEL,
}render_data_type_e;

typedef struct
{
    render_data_type_e type;

    uint32_t buffer_indices[MAX_SSBO_PER_SKINNED_MODEL];
    uint32_t buffer_count;

    uint32_t texture_indices[MAX_TEXTURES_PER_SKINNED_MODEL];
    uint32_t texture_count;
} render_data_config_t;

typedef enum 
{
    SHADER_STAGE_VERTEX,
    SHADER_STAGE_FRAGMENT,
}renderer_shader_stage_e;

typedef unsigned int renderer_config_flags_t;

typedef struct 
{
    const char                 *app_name;
    renderer_config_flags_t     flags;
}renderer_config_t;

typedef struct
{
    
}frame_data_t;

typedef struct
{
    renderer_shader_type_e type;
    void                   *internal_data;
}shader_t;

typedef struct
{
    vec3f_t position;
    vec3f_t front;
    vec3f_t up;
    vec3f_t right;
    vec3f_t world_up;

    float yaw;
    float pitch;

    float fov;
    float aspect;
    float znear;
    float zfar;
}camera_t;

/**
 * @brief uniform buffer + array of skinned model render data
 */
typedef struct 
{
    //! @brief scene uniforms
    void      *globals_data;
    //! @brief 
    void     **instance_data;
    uint32_t   instance_count;
}shader_resource_list_t;

/**
 * @brief For now we won't have a front-end structure to hold api-agnostic render data
 *        we might need one eventually
 */
typedef struct renderer_backend_t
{
    /**
     * @ brief api-specific rendering context
     */
    void *internal_context;

    /**
     * @brief initialize includes window swapchain creation too.
     */
    bool (*initialize)(struct renderer_backend_t *, window_t *, renderer_config_t *);
    void (*shutdown)(struct renderer_backend_t *);
    void (*window_destroy)(struct renderer_backend_t *, window_t *);
    bool (*frame_prepare)(struct renderer_backend_t *, frame_data_t *);
    bool (*begin_rendering)(struct renderer_backend_t *);
    bool (*end_rendering)(struct renderer_backend_t *);
    bool (*set_viewport)(struct renderer_backend_t *, float, float, float, float);
    bool (*set_scissor)(struct renderer_backend_t *,float, float, float, float);
    uint32_t (*get_num_frames_in_flight)(struct renderer_backend_t *);
    bool (*create_renderbuffer)(struct renderer_backend_t *, renderbuffer_t *, renderbuffer_type_e, uint8_t *, uint32_t);
    void (*copy_to_renderbuffer)(struct renderer_backend_t *, renderbuffer_t *, void *, uint32_t);
    bool (*bind_buffer)(struct renderer_backend_t *, renderbuffer_t*, shader_t*);
    bool (*create_shader)(struct renderer_backend_t *, shader_t *);
    bool (*use_shader)(struct renderer_backend_t *, shader_t *);
    bool (*shader_bind_resource)(struct renderer_backend_t *, shader_t *, render_data_type_e, void *);
    void*(*create_render_data)(struct renderer_backend_t *,render_data_config_t *config);
    bool (*initialize_shader)(struct renderer_backend_t *, shader_t *,  shader_resource_list_t *);
    bool (*create_texture)(struct renderer_backend_t *, texture_t *, const char *);
    bool (*bind_vertex_buffers)(struct renderer_backend_t *, renderbuffer_t *);
    bool (*bind_index_buffers)(struct renderer_backend_t *, renderbuffer_t *);
    bool (*frame_submit)(struct renderer_backend_t *, frame_data_t *);
    bool (*push_constants)(struct renderer_backend_t *, shader_t *, const void *,uint32_t, uint32_t, renderer_shader_stage_e);
    bool (*draw_indexed)(struct renderer_backend_t *,int32_t,uint32_t, uint32_t,uint32_t,uint32_t);
}renderer_backend_t;

typedef struct 
{
    renderer_backend_t *backend;
    shader_t           *current_shader;
    
    //scene matrices
    uint32_t  uniform_buffer_index;
    void     *uniform_buffer_render_data;
    
    //in permanent memory
    shader_t           *shaders;
    uint32_t            shader_count;

    camera_t            camera;

    //render target count
    uint32_t current_frame;
    uint32_t max_frames_in_flight;
}renderer_t;

#endif

