#ifndef BULK_DATA_TYPES_H
#define BULK_DATA_TYPES_H

#include <vulkan/vulkan.h>
#include <stdint.h>

#include "../math/math_types.h"

typedef uint64_t entity_flags_t;

#define MAX_ANIMATION_SAMPLER_INPUT_COUNT  64
#define MAX_ANIMATION_SAMPLER_OUTPUT_COUNT 64
#define MAX_ANIMATION_CHANNEL_COUNT        64
#define MAX_ANIMATIONS_PER_MODEL           32
#define MAX_TEXTURES_PER_MODEL             4
#define MAX_NODES_PER_MODEL                64
#define MAX_BONES_PER_SKIN                 256

//! TODO: Using multiple buffers for now. Switch to single buffer and multiple offsets 
#define MAX_BUFFERS_PER_RENDERBUFFER       3

typedef enum
{
    RENDERBUFFER_TYPE_VERTEX_BUFFER,
    RENDERBUFFER_TYPE_INDEX_BUFFER,
    RENDERBUFFER_TYPE_UNIFORM_BUFFER,
    RENDERBUFFER_TYPE_STAGING_BUFFER,
    RENDERBUFFER_TYPE_READ_BUFFER,
    RENDERBUFFER_TYPE_STORAGE_BUFFER
}renderbuffer_type_e;

//! NOTE: these are meant to be used as indices to descriptor set layout array in a shader
typedef enum
{
    //uniform buffer with global, per-frame or per-view data
    DESCRIPTOR_SET0,
    //uniform buffer and texture descriptors for per-material/object data (albedo map, fresnel coefficient etc)
    DESCRIPTOR_SET1,
}descriptor_set_type_e;

/**
 * @brief: API agnostic general purpose buffer structure
 * this is used for everything vulkan_buffer_t is used for i.e: uniform buffers, vertex/index buffers etc. 
 */
typedef struct
{
    renderbuffer_type_e   type;
    uint32_t              buffers[MAX_BUFFERS_PER_RENDERBUFFER];
    uint32_t              buffer_count;
    uint32_t              used;
    uint32_t              size;
}renderbuffer_t;

typedef struct
{
    void                  *internal_data;//api specific data
    descriptor_set_type_e  descriptor_set;
}texture_t;

typedef struct
{
    uint32_t        w,h;
    uint32_t        mip_levels;
    VkImage         image;
    VkImageView     view;
    VkSampler       sampler;
    VkImageLayout   layout;
    VkDeviceMemory  memory;
}vulkan_texture_t;

typedef struct 
{
    uint32_t  interpolation;

    float     inputs[MAX_ANIMATION_SAMPLER_INPUT_COUNT];
    uint32_t  input_count;

    vec4f_t   outputs[MAX_ANIMATION_SAMPLER_OUTPUT_COUNT];
    uint32_t  output_count;
} animation_sampler_t;

typedef struct 
{
    uint32_t path;   //enum
    uint32_t node;   //index
    uint32_t sampler;//index
} animation_channel_t;

typedef struct 
{
    //! NOTE: heap
    animation_sampler_t *samplers;
    uint32_t sampler_count;
    
    //! NOTE: heap
    animation_channel_t *channels;
    uint32_t channel_count;

    float start_time;
    float end_time;
    float current_time;
} animation_t;

//! @brief: all types of which a bulk_data_..._t should be declared here.
typedef enum
{
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_TILE,
    ENTITY_TYPE_ENEMY,
    ENTITY_TYPE_WEAPON,
    ENTITY_TYPE_WIDGET,
    ENTITY_TYPE_UNKNOWN
} entity_type_t;

typedef enum
{
    WIDGET_TYPE_FPS,
    WIDGET_TYPE_DIALOGUE,
    WIDGET_TYPE_DAMAGE_NUMBER,
    WIDGET_TYPE_MAX
} widget_type_t;

typedef enum
{
    ENTITY_STATE_IDLE,
    ENTITY_STATE_RUN,
    ENTITY_STATE_MELEE,
    ENTITY_STATE_RANGED,
    ENTITY_STATE_HIT,
    ENTITY_STATE_DEAD,
    ENTITY_STATE_MAX
} entity_state_t;

typedef enum
{
    WEAPON_SLOT_MAIN_HAND,
    WEAPON_SLOT_OFF_HAND,
    WEAPON_SLOT_TWO_HANDED,
    WEAPON_SLOT_MAX
} weapon_slot_t;

typedef struct
{  
    vulkan_texture_t *texture;
    rect_t glyphs[95];
} font_t;

typedef struct 
{
    const char   *text;
    font_t       *font;
    //from the owner entity
    vec2f_t       offset; 
    float         timer;
    //if it fades away, this is when it fades away, if it updates periodically, this is when it updates
    float         duration; 

} text_label_t;

typedef struct
{
    uint64_t     id;
    union {
        rect_t rect;
        struct{
            vec2f_t p;
            vec2f_t size;
        };
    };
    int32_t        z_index;
    entity_state_t state;
    entity_type_t  type;
    entity_flags_t flags;
    void           *data;
} entity_t;

typedef struct
{
    vulkan_texture_t *texture;
    rect_t            src_rect;
} sprite_t;

typedef struct 
{
    entity_t     *entity;
    sprite_t      sprite;
    weapon_slot_t slot;
} weapon_t;

typedef struct 
{
    entity_t     *entity;
    widget_type_t type;
    text_label_t  *text_label;
} widget_t;

typedef struct
{
    entity_t *entity;
    sprite_t sprite;
} tile_t;

typedef struct
{
    uint32_t    parent;
    uint32_t    index;
    quat_t      rotation;
    vec3f_t     translation;
    vec3f_t     scale;
    mat4f_t     local_transform;
    uint32_t    skin;
    uint32_t    mesh;
}model_node_t;

typedef struct 
{
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    void           *mapped;
    VkDeviceSize    buffer_size;
}vulkan_buffer_t;

typedef struct
{
    vec3f_t pos;
    vec3f_t normal;
    vec2f_t uv;
    //vec3f_t color;
    vec4f_t joint_indices;
    vec4f_t joint_weights;
} skinned_vertex_t;

typedef struct 
{
    vec4f_t  base_color_factor;
    uint32_t base_color_texture_index;
} material_t;

typedef struct 
{
    uint8_t   joints[MAX_BONES_PER_SKIN];
    mat4f_t   inverse_bind_matrices[MAX_BONES_PER_SKIN];
    uint32_t  joint_count;
    //bulk data
    uint32_t  ssbo;
    uint8_t   skeleton_root;
} skin_t;

typedef struct 
{
    uint32_t first_index;
    uint32_t index_count;
    uint32_t material;
}mesh_t;

typedef struct
{
    //! @brief rendering api specific data. i.e. descriptor sets for vulkan. allocated on MEM_TAG_HEAP
    void           *rendering_data;

    renderbuffer_t vertex_buffer;
    renderbuffer_t index_buffer;
    
    //! TODO: these come from bulk data (asset store)
    uint32_t       textures[MAX_TEXTURES_PER_MODEL];
    uint32_t       texture_count;

    //! animations are stored in struct
    animation_t    animations[MAX_ANIMATIONS_PER_MODEL];        
    uint32_t       animation_count;

    //! nodes are stored in struct
    model_node_t   nodes[MAX_NODES_PER_MODEL];
    uint32_t       node_count;

    uint8_t        skeleton_root;

    skin_t         skin;
    material_t     material;
    mesh_t         mesh;

    uint32_t       active_animation;
} skinned_model_t;


#endif