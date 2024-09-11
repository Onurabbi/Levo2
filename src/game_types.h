#ifndef GAME_TYPES_H_
#define GAME_TYPES_H_

#include <stdint.h>
#include <stddef.h>

#include "core/containers/container_types.h"
#include "core/asset_store/asset_store.h"

#define MAX_SPRITES_PER_ANIMATION 8
#define MAX_ANIMATIONS_PER_CHUNK  8
#define MAX_ANIMATION_CHUNKS_PER_ENTITY 4
#define MAX_VISIBLE_TEXT_PER_SCENE 32

#define ENTITY_CAN_COLLIDE 0x1
typedef uint64_t entity_flags_t;

enum 
{
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963
};

enum 
{
    SIGNED_BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SIGNED_SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
};

enum
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
};

enum
{
    POINTS = 0,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN
};

enum
{
    TRANSLATION,
    ROTATION,
    SCALE,
    WEIGHTS,
};

enum
{
    STEP_INTERPOLATION,
    LINEAR_INTERPOLATION,
    SPHERICAL_LINEAR_INTERPOLATION,
    CUBIC_SPLINE_INTERPOLATION
};

enum
{
    OPAQUE,
    MASK,
    BLEND,
};

enum
{
    NEAREST_FILTER = 9728,
    LINEAR_FILTER = 9729,
    NEAREST_MIPMAP_NEAREST_FILTER = 9984,
    LINEAR_MIPMAP_NEAREST_FILTER = 9985,
    NEAREST_MIPMAP_LINEAR_FILTER = 9986,
    LINEAR_MIPMAP_LINEAR_FILTER = 9986
};

enum
{
    CLAMP_TO_EDGE = 33071,
    MIRRORED_REPEAT = 33648,
    REPEAT = 10497
};

typedef struct
{
    //vertex attributes
    uint32_t joints;
    uint32_t normal;
    uint32_t position;
    uint32_t tex_coord;
    uint32_t weights;

    //
    uint32_t indices;
    uint32_t mode;
    uint32_t material;
}gltf_mesh_primitive_t;

typedef struct
{
    const char            *name; //is this required
    gltf_mesh_primitive_t  primitive;
}gltf_mesh_t;

typedef struct
{
    uint32_t input;
    uint32_t output;
    uint32_t interpolation;
}gltf_animation_sampler_t;

typedef struct
{
    uint32_t node;
    uint32_t path;
    uint32_t sampler;
}gltf_channel_t;

typedef struct
{
    gltf_channel_t           *channels;
    uint32_t                  channel_count;

    gltf_animation_sampler_t *samplers;
    uint32_t                  sampler_count;
}gltf_animation_t;

typedef struct
{
    const char *name;

    uint32_t    *joints;
    uint32_t     joint_count;

    uint32_t     inverse_bind_matrices;
    uint32_t     skeleton;
}gltf_skin_t;

typedef struct
{
    uint8_t *data;
    uint32_t size;
}gltf_buffer_t;

typedef struct 
{
    const char *name;

    uint32_t  children[4];
    uint32_t  child_count;

    mat4f_t  local_transform;

    uint32_t  skin;
    uint32_t  mesh;
}gltf_node_t; 

typedef struct
{
    uint32_t    parent; 
    uint32_t    index;
    mat4f_t     local_transform;
    uint32_t    skin;
    uint32_t    mesh;
}model_node_t;

typedef struct
{
    uint32_t  buffer_view;
    uint32_t  byte_offset;
    uint32_t  count;
    uint32_t  component_type; //index or vertex
    uint32_t  type;//scalar, vector etc.
}gltf_accessor_t;

typedef struct
{
    uint32_t buffer;
    uint32_t byte_offset;
    uint32_t byte_length;
    uint32_t byte_stride; //sparse data layout
    uint32_t target;
}gltf_buffer_view_t;

typedef struct 
{
    uint32_t *nodes;
    uint32_t  node_count;
} gltf_scene_t;

typedef struct 
{
    uint32_t base_color_texture_index;
    uint32_t tex_coord;
    
    float    metallic_factor;
    float    roughness_factor;
    vec4f_t  base_color_factor;

}gltf_pbr_metallic_roughness_t;

typedef struct
{
    const char *name;
    gltf_pbr_metallic_roughness_t pbr_metallic_roughness;
    vec3f_t emissive_factor;
    uint32_t alpha_mode;
    bool double_sided;
}gltf_material_t;


typedef struct 
{
    uint32_t sampler;
    uint32_t source;
}gltf_texture_t;

typedef struct 
{
    uint32_t mag_filter;
    uint32_t min_filter;
    uint32_t wrap_s;
    uint32_t wrap_t;
}gltf_sampler_t;

typedef struct
{
    const char         *path;
    
    gltf_scene_t       *scenes;
    uint32_t            scene_count;

    gltf_node_t        *nodes;
    uint32_t            node_count;

    gltf_buffer_view_t *buffer_views;
    uint32_t            buffer_view_count;

    gltf_accessor_t    *accessors;
    uint32_t            accessor_count;

    gltf_buffer_t      *buffers;
    uint32_t            buffer_count;

    gltf_animation_t   *animations;
    uint32_t            animation_count;

    gltf_skin_t        *skins;
    uint32_t            skin_count;

    gltf_mesh_t        *meshes;
    uint32_t            mesh_count;

    uint32_t           *vertex_counts;         //one per mesh
    size_t             *vertices_byte_lengths; //one per mesh
    uint32_t           *index_counts;          //one per mesh
    size_t             *indices_byte_lengths;  //one per mesh

    gltf_material_t    *materials;
    uint32_t            material_count;

    gltf_texture_t     *textures;
    uint32_t            texture_count;

    const char        **image_paths;
    uint32_t            image_count;

    gltf_sampler_t     *samplers;
    uint32_t            sampler_count;

    uint32_t            vertex_buffer;
    uint32_t            index_buffer;
} gltf_model_t;

typedef struct 
{
    uint32_t  interpolation;

    float    *inputs;
    uint32_t  input_count;

    vec4f_t  *outputs;
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
    animation_sampler_t *samplers;
    uint32_t sampler_count;

    animation_channel_t *channels;
    uint32_t channel_count;

    float start_time;
    float end_time;
    float current_time;
} animation_t;

typedef struct
{
    vec3f_t pos;
    vec3f_t normal;
    vec2f_t uv;
    //vec3f_t color;
    vec4i_t joint_indices;
    vec4f_t joint_weights;
} skinned_vertex_t;

typedef struct 
{
    vec4f_t  base_color_factor;
    uint32_t base_color_texture_index;
} material_t;

typedef struct 
{
    uint32_t *joints;
    uint32_t  joint_count;

    mat4f_t  *inverse_bind_matrices;
    uint32_t  inverse_bind_matrix_count;

    vulkan_buffer_t  ssbo;

} skin_t;

typedef struct
{
    vulkan_texture_t **textures;
    uint32_t           texture_count;

    material_t        *materials;
    uint32_t           material_count;

    model_node_t      *nodes;
    uint32_t           node_count;

    skin_t            *skins;
    uint32_t           skin_count;

    animation_t       *animations;
    uint32_t           animation_count;
} model_t;

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
    entity_state_t    state;
    vulkan_texture_t *texture;

    rect_t            sprites[MAX_SPRITES_PER_ANIMATION];
    uint32_t          sprite_count;
    float             duration;
} sprite_animation_t;

typedef struct 
{
    sprite_animation_t animations[MAX_ANIMATIONS_PER_CHUNK];
    uint32_t           count;
} animation_chunk_t;

typedef struct 
{
    animation_t *animation;
    float        timer;
} animation_update_result_t;

typedef struct 
{
    entity_t    *entity;
    
    text_label_t *name;

    float        velocity;
    vec2f_t      dp;

    //animation playback
    animation_chunk_t *animation_chunks[MAX_ANIMATION_CHUNKS_PER_ENTITY];
    animation_t*       current_animation;
    uint32_t           animation_chunk_count;
    float              anim_timer;

    entity_t          *weapon;
} player_t;

typedef struct
{
    entity_t *entity;
    sprite_t sprite;
} tile_t;

typedef struct
{
    uint32_t i;
    float    f;
} uint_float_pair;

BulkDataTypes(entity_t)
BulkDataTypes(tile_t)
BulkDataTypes(animation_chunk_t)
BulkDataTypes(sprite_t)
BulkDataTypes(weapon_t)
BulkDataTypes(widget_t)
BulkDataTypes(text_label_t)

struct SDL_Window;

typedef struct 
{
    const uint8_t *keyboard_state;
    int32_t        mouse_x,mouse_y;
    bool           quit;
} input_t;

typedef struct 
{
    struct SDL_Window *window;
    int32_t            window_width;
    int32_t            window_height;
    float              tile_width;

    asset_store_t      asset_store;
    vulkan_renderer_t  renderer;
    
    //one player per game
    player_t           player_data;
    entity_t          *player_entity;

    uint64_t           entity_id;
    bulk_data_entity_t entities;

    //entity specific data (other than player)
    bulk_data_tile_t       tiles;
    bulk_data_weapon_t     weapons;
    bulk_data_widget_t     widgets;

    //resources
    bulk_data_animation_chunk_t animation_chunks;
    bulk_data_sprite_t          sprites;
    bulk_data_text_label_t      text_labels;

    //text rendering
    font_t font;

    input_t input;
    rect_t  camera;
    //timing
    uint64_t          previous_counter;
    uint64_t          performance_freq;
    double            accumulator;

    bool    is_running;
} game_t;


#endif

