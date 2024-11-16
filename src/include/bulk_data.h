#ifndef BULK_DATA_H
#define BULK_DATA_H


#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#include "bulk_data_types.h"

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

#endif
