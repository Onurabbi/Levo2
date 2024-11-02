#include <string.h>
#include "../memory/memory.h"
#include "bulk_data_types.h"

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

typedef struct
{
	item_entity_t *items;
	uint32_t count;
} bulk_data_entity_t;

void bulk_data_delete_item_entity_t(bulk_data_entity_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_entity_t(bulk_data_entity_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

entity_t *bulk_data_getp_null_entity_t(bulk_data_entity_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_entity_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_entity_t(bulk_data_entity_t *bd, entity_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_entity_t(bulk_data_entity_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_entity_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
typedef struct
{
	tile_t data;
}object_tile_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_tile_t;
		freelist_item_t;
	};
}item_tile_t;

typedef struct
{
	item_tile_t *items;
	uint32_t count;
} bulk_data_tile_t;

void bulk_data_delete_item_tile_t(bulk_data_tile_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_tile_t(bulk_data_tile_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

tile_t *bulk_data_getp_null_tile_t(bulk_data_tile_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_tile_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_tile_t(bulk_data_tile_t *bd, tile_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_tile_t(bulk_data_tile_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_tile_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
typedef struct
{
	sprite_t data;
}object_sprite_t;

typedef struct
{
	data_type_e data_type;
	uint32_t generation;
	union {
		object_sprite_t;
		freelist_item_t;
	};
}item_sprite_t;

typedef struct
{
	item_sprite_t *items;
	uint32_t count;
} bulk_data_sprite_t;

void bulk_data_delete_item_sprite_t(bulk_data_sprite_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_sprite_t(bulk_data_sprite_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

sprite_t *bulk_data_getp_null_sprite_t(bulk_data_sprite_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_sprite_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_sprite_t(bulk_data_sprite_t *bd, sprite_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_sprite_t(bulk_data_sprite_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_sprite_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_weapon_t *items;
	uint32_t count;
} bulk_data_weapon_t;

void bulk_data_delete_item_weapon_t(bulk_data_weapon_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_weapon_t(bulk_data_weapon_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

weapon_t *bulk_data_getp_null_weapon_t(bulk_data_weapon_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_weapon_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_weapon_t(bulk_data_weapon_t *bd, weapon_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_weapon_t(bulk_data_weapon_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_weapon_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_widget_t *items;
	uint32_t count;
} bulk_data_widget_t;

void bulk_data_delete_item_widget_t(bulk_data_widget_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_widget_t(bulk_data_widget_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

widget_t *bulk_data_getp_null_widget_t(bulk_data_widget_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_widget_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_widget_t(bulk_data_widget_t *bd, widget_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_widget_t(bulk_data_widget_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_widget_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_text_label_t *items;
	uint32_t count;
} bulk_data_text_label_t;

void bulk_data_delete_item_text_label_t(bulk_data_text_label_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_text_label_t(bulk_data_text_label_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

text_label_t *bulk_data_getp_null_text_label_t(bulk_data_text_label_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_text_label_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_text_label_t(bulk_data_text_label_t *bd, text_label_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_text_label_t(bulk_data_text_label_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_text_label_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_texture_t *items;
	uint32_t count;
} bulk_data_texture_t;

void bulk_data_delete_item_texture_t(bulk_data_texture_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_texture_t(bulk_data_texture_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

texture_t *bulk_data_getp_null_texture_t(bulk_data_texture_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_texture_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_texture_t(bulk_data_texture_t *bd, texture_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_texture_t(bulk_data_texture_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_texture_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_vulkan_texture_t *items;
	uint32_t count;
} bulk_data_vulkan_texture_t;

void bulk_data_delete_item_vulkan_texture_t(bulk_data_vulkan_texture_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_vulkan_texture_t(bulk_data_vulkan_texture_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

vulkan_texture_t *bulk_data_getp_null_vulkan_texture_t(bulk_data_vulkan_texture_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_vulkan_texture_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_vulkan_texture_t(bulk_data_vulkan_texture_t *bd, vulkan_texture_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_vulkan_texture_t(bulk_data_vulkan_texture_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_vulkan_texture_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_vulkan_buffer_t *items;
	uint32_t count;
} bulk_data_vulkan_buffer_t;

void bulk_data_delete_item_vulkan_buffer_t(bulk_data_vulkan_buffer_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_vulkan_buffer_t(bulk_data_vulkan_buffer_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

vulkan_buffer_t *bulk_data_getp_null_vulkan_buffer_t(bulk_data_vulkan_buffer_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_vulkan_buffer_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_vulkan_buffer_t(bulk_data_vulkan_buffer_t *bd, vulkan_buffer_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_vulkan_buffer_t(bulk_data_vulkan_buffer_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_vulkan_buffer_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_skinned_model_t *items;
	uint32_t count;
} bulk_data_skinned_model_t;

void bulk_data_delete_item_skinned_model_t(bulk_data_skinned_model_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_skinned_model_t(bulk_data_skinned_model_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

skinned_model_t *bulk_data_getp_null_skinned_model_t(bulk_data_skinned_model_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_skinned_model_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_skinned_model_t(bulk_data_skinned_model_t *bd, skinned_model_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_skinned_model_t(bulk_data_skinned_model_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_skinned_model_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
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

typedef struct
{
	item_renderbuffer_t *items;
	uint32_t count;
} bulk_data_renderbuffer_t;

void bulk_data_delete_item_renderbuffer_t(bulk_data_renderbuffer_t *bd, uint32_t i)
{
	if (i >= bd->count) return;
	bd->items[i].next = bd->items[0].next;
	bd->items[i].data_type = FREELIST_ITEM;
	bd->items[i].generation++;
	bd->items[0].next = i;
}

uint32_t bulk_data_allocate_slot_renderbuffer_t(bulk_data_renderbuffer_t *bd)
{
	uint32_t slot = bd->items[0].next;
	bd->items[0].next = bd->items[slot].next;
	if (slot) {
		bd->items[slot].data_type = OBJECT_ITEM;
		return slot;
	}
	slot = bd->count++;
	bd->items[slot].data_type = OBJECT_ITEM;
	return slot;
}

renderbuffer_t *bulk_data_getp_null_renderbuffer_t(bulk_data_renderbuffer_t *bd, uint32_t i)
{
	if (i > bd->count) return NULL;
	item_renderbuffer_t *it = &bd->items[i];
	if (it->data_type == FREELIST_ITEM) return NULL;
	return &it->data;
}

uint32_t bulk_data_index_renderbuffer_t(bulk_data_renderbuffer_t *bd, renderbuffer_t *ptr)
{
	uint32_t index = (ptr - &bd->items[0].data);
	return index;
}

void bulk_data_init_renderbuffer_t(bulk_data_renderbuffer_t *bd)
{
	memset(bd, 0, sizeof(*bd));
	bd->items = memory_alloc(GIGABYTES(1), MEM_TAG_BULK_DATA);
	item_renderbuffer_t *dummy = &bd->items[bd->count++];
	dummy->next = 0;
	dummy->generation = 0;
}
