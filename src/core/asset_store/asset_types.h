#ifndef ASSET_TYPES_H_
#define ASSET_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stb/stb_ds.h>
#include "../containers/containers.h"
#include "../renderer/frontend/renderer_types.h"
#include "../math/math_types.h"

#define ENTITY_CAN_COLLIDE 0x1

typedef enum 
{
    ARRAY_BUFFER = 34962,
    ELEMENT_ARRAY_BUFFER = 34963
}buffer_view_target_e;

typedef enum 
{
    SIGNED_BYTE = 5120,
    UNSIGNED_BYTE = 5121,
    SIGNED_SHORT = 5122,
    UNSIGNED_SHORT = 5123,
    UNSIGNED_INT = 5125,
    FLOAT = 5126
}accessor_component_type_e;

typedef enum
{
    SCALAR,
    VEC2,
    VEC3,
    VEC4,
    MAT2,
    MAT3,
    MAT4
}accessor_data_type_e;

typedef enum
{
    POINTS = 0,
    LINES,
    LINE_LOOP,
    LINE_STRIP,
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN
}primitive_topology_e;

typedef enum
{
    TRANSLATION,
    ROTATION,
    SCALE,
    WEIGHTS,
}animation_channel_path_e;

typedef enum
{
    STEP_INTERPOLATION,
    LINEAR_INTERPOLATION,
    SPHERICAL_LINEAR_INTERPOLATION,
    CUBIC_SPLINE_INTERPOLATION
}animation_sampler_interpolation_e;

typedef enum
{
    OPAQUE,
    MASK,
    BLEND,
}alpha_mode_e;

typedef enum
{
    NEAREST_FILTER = 9728,
    LINEAR_FILTER = 9729,
    NEAREST_MIPMAP_NEAREST_FILTER = 9984,
    LINEAR_MIPMAP_NEAREST_FILTER = 9985,
    NEAREST_MIPMAP_LINEAR_FILTER = 9986,
    LINEAR_MIPMAP_LINEAR_FILTER = 9986
}sampler_filter_e;

typedef enum
{
    CLAMP_TO_EDGE = 33071,
    MIRRORED_REPEAT = 33648,
    REPEAT = 10497
}sampler_wrap_e;

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
    gltf_mesh_primitive_t  primitives[4];
    uint32_t               primitive_count;
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

    uint8_t     *joints;
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

    mat4f_t   local_transform;
    vec3f_t   translation;
    vec3f_t   scale;
    quat_t    rotation;
    uint32_t  skin;
    uint32_t  mesh;
}gltf_node_t; 

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
    const char         *asset_id;
    
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

typedef enum {
    ASSET_TYPE_TEXTURE,
    ASSET_TYPE_SKINNED_MODEL,
}asset_type_e;

//! NOTE: this structure only holds indices to bulk data
typedef struct 
{
    //api agnostic texture data
    string_hash_entry_t        *texture_map;
    bulk_data_texture_t        *textures;
    //data related to skinned models
    string_hash_entry_t        *skinned_model_map;
    bulk_data_skinned_model_t  *skinned_models;
}asset_store_t;


#endif

