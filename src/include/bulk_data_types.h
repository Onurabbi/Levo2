#ifndef BULK_DATA_TYPES_H
#define BULK_DATA_TYPES_H

#include <asset_types.h>
#include <renderer_types.h>
#include <vulkan_types.h>
#include <game_types.h>
#include <stdint.h>

typedef struct freelist_item_t
{
    uint32_t next;
}freelist_item_t;

typedef enum data_type_e
{
    FREELIST_ITEM,
    OBJECT_ITEM,
}data_type_e;
typedef struct 
{
	entity_t data;
}object_entity_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_entity_t;
		freelist_item_t;
	};
}item_entity_t;

typedef struct bulk_data_entity_t
{
	item_entity_t *items;
	uint32_t count;
} bulk_data_entity_t;

typedef struct 
{
	weapon_t data;
}object_weapon_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_weapon_t;
		freelist_item_t;
	};
}item_weapon_t;

typedef struct bulk_data_weapon_t
{
	item_weapon_t *items;
	uint32_t count;
} bulk_data_weapon_t;

typedef struct 
{
	widget_t data;
}object_widget_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_widget_t;
		freelist_item_t;
	};
}item_widget_t;

typedef struct bulk_data_widget_t
{
	item_widget_t *items;
	uint32_t count;
} bulk_data_widget_t;

typedef struct 
{
	text_label_t data;
}object_text_label_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_text_label_t;
		freelist_item_t;
	};
}item_text_label_t;

typedef struct bulk_data_text_label_t
{
	item_text_label_t *items;
	uint32_t count;
} bulk_data_text_label_t;

typedef struct 
{
	texture_t data;
}object_texture_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_texture_t;
		freelist_item_t;
	};
}item_texture_t;

typedef struct bulk_data_texture_t
{
	item_texture_t *items;
	uint32_t count;
} bulk_data_texture_t;

typedef struct 
{
	vulkan_texture_t data;
}object_vulkan_texture_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_vulkan_texture_t;
		freelist_item_t;
	};
}item_vulkan_texture_t;

typedef struct bulk_data_vulkan_texture_t
{
	item_vulkan_texture_t *items;
	uint32_t count;
} bulk_data_vulkan_texture_t;

typedef struct 
{
	vulkan_buffer_t data;
}object_vulkan_buffer_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_vulkan_buffer_t;
		freelist_item_t;
	};
}item_vulkan_buffer_t;

typedef struct bulk_data_vulkan_buffer_t
{
	item_vulkan_buffer_t *items;
	uint32_t count;
} bulk_data_vulkan_buffer_t;

typedef struct 
{
	skinned_model_t data;
}object_skinned_model_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_skinned_model_t;
		freelist_item_t;
	};
}item_skinned_model_t;

typedef struct bulk_data_skinned_model_t
{
	item_skinned_model_t *items;
	uint32_t count;
} bulk_data_skinned_model_t;

typedef struct 
{
	renderbuffer_t data;
}object_renderbuffer_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_renderbuffer_t;
		freelist_item_t;
	};
}item_renderbuffer_t;

typedef struct bulk_data_renderbuffer_t
{
	item_renderbuffer_t *items;
	uint32_t count;
} bulk_data_renderbuffer_t;

typedef struct bulk_data_t {
	bulk_data_entity_t entities;
	bulk_data_weapon_t weapons;
	bulk_data_widget_t widgets;
	bulk_data_text_label_t text_labels;
	bulk_data_texture_t textures;
	bulk_data_vulkan_texture_t vulkan_textures;
	bulk_data_vulkan_buffer_t vulkan_buffers;
	bulk_data_skinned_model_t skinned_models;
	bulk_data_renderbuffer_t renderbuffers;
}bulk_data_t;

#endif
