#include "generator.h"
#include "../logger/logger.h"
#include "collision.h"
#include "../memory/memory.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

float rng_in_range(float min, float max)
{
    double scale =  rand() / (double)RAND_MAX;
    return (float)(min + scale * (max - min));
}

void create_dungeon(void)
{
    uint32_t max_tries = 250;
    uint32_t num_rooms = 50;

    float map_width = 128.0f;
    float map_height = 128.0f;

    uint32_t room_count = 0;
    rect_t rooms[50];
    uint8_t grid[128][128];
    memset(grid, 0, sizeof(grid));

    rect_t *first_room = &rooms[room_count++];
    first_room->min.x = 0;
    first_room->min.y = 0;
    first_room->size.x = 25;
    first_room->size.y = 25;
    for (uint32_t row = 0; row < 25; row++)
    {
        for (uint32_t col = 0; col < 25; col++)
        {
            grid[row][col] = 1;
        }
    }
    for (uint32_t i = 0; i < max_tries; i++)
    {
        float width = rng_in_range(10.0f, 18.0f);
        float height = rng_in_range(10.0f, 18.0f);

        float x = rng_in_range(0, map_width - width);
        float y = rng_in_range(0, map_height - height);

        rect_t room = (rect_t){{x,y},{width, height}};
        bool collided = false;
        for (uint32_t j = 0; j < room_count; j++)
        {
            rect_t other = rooms[j];
            if (rect_vs_rect(room, other))
            {
                collided = true;
                break;
            }
        }
        if (collided) continue;

        //we were able to place the room;
        rooms[room_count++] = room;
        //now fill the grid points where the room occupies
        int32_t room_min_x = floorf(x);
        int32_t room_max_x = floorf(x + width);
        int32_t room_min_y = floorf(y);
        int32_t room_max_y = floorf(y + height);
#if 0
        room_min_x = room_min_x < 0 ? 0 : room_min_x;
        room_min_y = room_min_y < 0 ? 0 : room_min_y;
        room_max_x = room_max_x > 127 ? 127 : room_max_x;
        room_max_y = room_max_y > 127 ? 127 : room_max_y; 
#endif
        for (uint32_t row = room_min_y; row < room_max_y; row++)
        {
            for (uint32_t col = room_min_x; col < room_max_x; col++)
            {
                grid[row][col] = 1;
            }
        }
        if (room_count == num_rooms) break;
    }

    //now write to file
    FILE *test_map = fopen("./assets/tilemaps/dungeon.map", "w");
    char *buf = memory_alloc(1024*1024, MEM_TAG_TEMP);
    char *ptr = buf;
    for (uint32_t row = 0; row  < 128; row++)
    {
        for (uint32_t col = 0; col < 128; col++)
        {
            int val = 11;
            if (grid[row][col]) val = 140;
            ptr += sprintf(ptr, "%03d,",val);
        }
        ptr += sprintf(ptr, "\n");
    }
    fwrite(buf, 1, (size_t)(ptr - buf), test_map);
    fclose(test_map);
    LOGI("SUCCESS");
}
