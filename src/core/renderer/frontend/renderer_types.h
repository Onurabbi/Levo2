#ifndef RENDERER_TYPES_H_
#define RENDERER_TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include "../../containers/container_types.h"
#include "../../platform/platform.h"

typedef enum
{
    DESCRIPTOR_TYPE_UNIFORM_BUFFER,
}renderer_descriptor_type_e;

typedef enum
{
    DESCRIPTOR_BINDING_VERTEX_SHADER_STAGE,
    DESCRIPTOR_BINDING_FRAGMENT_SHADER_STAGE,
}renderer_descriptor_binding_e;

typedef enum
{
    RENDERBUFFER_TYPE_VERTEX_BUFFER,
    RENDERBUFFER_TYPE_INDEX_BUFFER,
    RENDERBUFFER_TYPE_UNIFORM_BUFFER,
    RENDERBUFFER_TYPE_STAGING_BUFFER,
    RENDERBUFFER_TYPE_READ_BUFFER,
    RENDERBUFFER_TYPE_STORAGE_BUFFER
}renderbuffer_type_e;

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

typedef unsigned int renderer_config_flags_t;

typedef struct 
{
    const char              *app_name;
    renderer_config_flags_t flags;
}renderer_config_t;

typedef struct
{
    
}frame_data_t;

typedef struct
{
    void *internal_data;
}shader_t;

typedef struct
{
    void *internal_data;//api specific data
}texture_t;

/**
 * @brief: API agnostic general purpose buffer structure
 * this is used for everything vulkan_buffer_t is used for i.e: uniform buffers, vertex/index buffers etc. 
 */
typedef struct
{
    //! Api specific data
    void *internal_data;

    renderbuffer_type_e type;
    uint32_t            used;
    uint32_t            size;
}renderbuffer_t;

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
    bool (*initialize)(struct renderer_backend_t *renderer, window_t *window, renderer_config_t *config);
    void (*shutdown)(struct renderer_backend_t *renderer);
    void (*window_destroy)(struct renderer_backend_t *renderer, window_t *window);
    bool (*frame_prepare)(struct renderer_backend_t *renderer, frame_data_t *frame_data);
    bool (*begin_rendering)(struct renderer_backend_t *renderer);
    bool (*end_rendering)(struct renderer_backend_t *renderer);
    bool (*set_viewport)(struct renderer_backend_t *renderer, float w, float h, float minz, float maxz);
    bool (*set_scissor)(struct renderer_backend_t *renderer,float x, float y, float w, float h);
    uint32_t (*get_num_frames_in_flight)(struct renderer_backend_t *renderer);
    bool (*create_renderbuffer)(struct renderer_backend_t *renderer, renderbuffer_t *renderbuffer, renderbuffer_type_e type,shader_t *shader, uint8_t *buffer_data, uint32_t size);
    void (*copy_to_renderbuffer)(struct renderer_backend_t *renderer, renderbuffer_t *renderbuffer, void *data, uint32_t size);
    bool (*create_shader)(struct renderer_backend_t *renderer, shader_t *shader);
    bool (*use_shader)(struct renderer_backend_t *renderer, shader_t * shader);
    bool (*create_texture)(struct renderer_backend_t *renderer, texture_t *texture, const char *);
    bool (*bind_vertex_buffers)(struct renderer_backend_t *renderer, renderbuffer_t *vertex_buffer);
    bool (*bind_index_buffers)(struct renderer_backend_t *renderer, renderbuffer_t *index_buffer);
}renderer_backend_t;

typedef struct 
{
    renderer_backend_t *backend;
    
    renderbuffer_t     *renderbuffers;
    uint32_t            renderbuffer_count;

    shader_t           *shaders;

    //textures
    //render target count
    //vertex buffer
    //index buffer
    uint32_t current_frame;
    uint32_t max_frames_in_flight;
}renderer_t;

BulkDataTypes(texture_t);

#endif

