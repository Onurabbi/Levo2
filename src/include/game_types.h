#ifndef GAME_TYPES_H_
#define GAME_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <vulkan_types.h>
#include <asset_store.h>

#define MAX_VISIBLE_TEXT_PER_SCENE         32

struct SDL_Window;

typedef struct 
{
    const uint8_t *keyboard_state;
    int32_t        mouse_x,mouse_y;
    bool           quit;
} input_t;

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

typedef uint64_t entity_flags_t;

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
    entity_t     *entity;
    text_label_t *name;

    float         velocity;
    vec2f_t       dp;

    uint32_t      animation_chunk_count;
    float         anim_timer;

    entity_t     *weapon;
} player_t;

typedef struct
{
    uint32_t i;
    float    f;
} uint_float_pair;

typedef struct 
{
    entity_t     *entity;
    weapon_slot_t slot;
} weapon_t;

typedef struct 
{
    entity_t     *entity;
    widget_type_t type;
    text_label_t  *text_label;
} widget_t;

#endif

