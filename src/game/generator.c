#include "generator.h"
#include "../logger/logger.h"
#include "collision.h"
#include "../memory/memory.h"
#include "../math/math_types.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define GRID_SIZE                 127
#define ROOM_COUNT                50
#define QUEUE_CAP                 2048
#define MAX_CONNECTORS_PER_REGION 72 //19 * 2  + 17 * 2

static vec2i_t cardinal[4] = {{-1,0}, {0,1}, {0,-1}, {1,0}};

enum
{
    WALL = 0x1,
    FLOOR = 0x2,
    INVALID_TILE = 0x0
};

typedef struct __region_connector_t
{
    struct __region_connector_t *next;
    vec2i_t p;
    uint8_t region; //which region does this connector connect to?
}region_connector_t;

typedef struct 
{
    uint8_t tile;
    uint8_t region;
}grid_tile_t;

typedef struct 
{
    grid_tile_t *data;
    int32_t width;
    int32_t height;
    int32_t region_count;
}grid_t;

static float rng_in_range(float min, float max)
{
    double scale =  rand() / (double)RAND_MAX;
    return (float)(min + scale * (max - min));
}

static inline uint8_t grid_get_region(grid_t *grid, int32_t row, int32_t col)
{
    if (row < 0 || row >= grid->height || col < 0 || col >= grid->width) return 0;
    return (grid->data[grid->width * row + col]).region;
}

static inline uint8_t grid_get_tile(grid_t *grid, int32_t row, int32_t col)
{
    if (row < 0 || row >= grid->height || col < 0 || col >= grid->width) return INVALID_TILE;
    return (grid->data[grid->width * row + col]).tile;
}

static inline void grid_set_region(grid_t *grid, int32_t row, int32_t col, uint8_t val)
{
    if (row < 0 || row >= grid->height || col < 0 || col >= grid->width)
    {
        assert(false && "out of bounds grid_set");
    }
    grid->data[grid->width * row + col].region = val;
}

static inline void grid_set_tile(grid_t *grid, int32_t row, int32_t col, uint8_t val)
{
    if (row < 0 || row >= grid->height || col < 0 || col >= grid->width)
    {
        assert(false && "out of bounds grid_set");
    }
    grid->data[grid->width * row + col].tile = val;
}

static inline void grid_set(grid_t *grid, int32_t row, int32_t col, grid_tile_t val)
{
    if (row < 0 || row >= grid->height || col < 0 || col >= grid->width)
    {
        assert(false && "out of bounds grid_set");
    }
    grid->data[grid->width * row + col] = val;
}


static inline void grid_init(grid_t *grid, int32_t width, int32_t height)
{
    grid->width = width;
    grid->height = height;
    grid->region_count = 0;

    grid->data = memory_alloc(sizeof(grid_tile_t) * grid->width * grid->height, MEM_TAG_TEMP);//already memset'd
    for (uint32_t row = 0; row < grid->height; row++)
    {
        for (uint32_t col = 0; col < grid->width; col++)
        {
            grid_set_tile(grid, row, col, WALL);
            grid_set_region(grid, row, col, 0);
        }
    }
}

static bool can_carve(grid_t *grid, vec2i_t p, vec2i_t dir)
{
    int x1 = p.x;
    int x2 = p.x + dir.x * 3;
    int y1 = p.y;
    int y2 = p.y + dir.y * 3;

    if (x1 < 0 || x1 >= GRID_SIZE ||
        x2 < 0 || x2 >= GRID_SIZE ||
        y1 < 0 || y1 >= GRID_SIZE ||
        y2 < 0 || y2 >= GRID_SIZE)
    {
        return false;
    }

    int x = p.x + dir.x * 2;
    int y = p.y + dir.y * 2;
    return (grid_get_tile(grid, y,x) == WALL);
}

static void grow_maze(grid_t *grid, int32_t x, int32_t y)
{
    grid->region_count++;

    vec2i_t queue[QUEUE_CAP];
    uint32_t queue_count = 0;
    memset(queue, 0, sizeof(queue));

    vec2i_t start = (vec2i_t){x,y};
    queue[queue_count++] = start;

    while(queue_count > 0)
    {
        vec2i_t current = queue[queue_count - 1];

        vec2i_t directions[4];
        uint32_t direction_count = 0;

        for (uint32_t i = 0; i < sizeof(cardinal)/sizeof(cardinal[0]); i++)
        {
            if (can_carve(grid, current, cardinal[i]))
            {
               directions[direction_count++] = cardinal[i];
            }
        }
        if (direction_count > 0)
        {
            assert(queue_count < QUEUE_CAP);

            int32_t dir_index = rand() % direction_count;
            vec2i_t dir = directions[dir_index];

            grid_set(grid, current.y + dir.y, current.x + dir.x, (grid_tile_t){FLOOR, grid->region_count});
            grid_set(grid, current.y + dir.y * 2, current.x + dir.x * 2, (grid_tile_t){FLOOR, grid->region_count});
            
            queue[queue_count++] = (vec2i_t){current.x + dir.x * 2, current.y + dir.y * 2};
        }
        else
        {
            queue_count--;
        }
    }
}

static region_connector_t *append_connector_to_tail(region_connector_t *head, region_connector_t *node)
{
    if (!head)
    {
        head = node;
    }
    else
    {
        region_connector_t *p = head;
        while(p->next)
        {
            p = p->next;
        }
        p->next = node;
    }
    return head;
}

//we will need to keep deleting until this returns false
static bool delete_connector(region_connector_t **head, uint8_t region)
{
    region_connector_t *temp = *head;
    region_connector_t *prev = NULL;

    if (temp && temp->region == region)
    {
        *head = temp->next;
        return true;
    }

    while (temp && temp->region != region)
    {
        prev = temp;
        temp = temp->next;
    }

    if (!temp) return false;

    prev->next = temp->next;
    return true;
}

static void connect_regions(grid_t *grid)
{
    region_connector_t ** region_connectors = memory_alloc(grid->region_count * sizeof(region_connector_t*), MEM_TAG_TEMP); //array of linked lists
    for (uint32_t i = 0; i < grid->region_count; i++)
    {
        region_connectors[i] = NULL;
    }

    for (uint32_t row = 1; row < grid->height - 1; row++)
    {
        for (uint32_t col = 1; col < grid->width - 1; col++)
        {
            if (grid_get_tile(grid, row, col) == WALL)
            {
                uint8_t from = 0; //index into the region_connectors array
                uint8_t to   = 0; //region field in region_connector_t struct

                for (uint32_t i = 0; i < 4; i++)
                {
                    uint8_t region = grid_get_region(grid, row + cardinal[i].y, col + cardinal[i].x);
                    uint8_t tile   = grid_get_tile(grid, row + cardinal[i].y, col + cardinal[i].x);
                    if (tile == FLOOR &&  region != 0)
                    {
                        if (from == 0)
                        {
                            from = region;
                        }
                        else if (to == 0 && region != from )
                        {
                            to = region;
                        }
                    }

                    if (from && to)
                    {
                        //create two nodes, add one to 'from' list, one to 'to' list
                        region_connector_t *conn_from = memory_alloc(sizeof(region_connector_t), MEM_TAG_TEMP);
                        conn_from->next = NULL;
                        conn_from->region = to;
                        conn_from->p = (vec2i_t){col,row};
                        region_connectors[from - 1] = append_connector_to_tail(region_connectors[from - 1], conn_from);

                        region_connector_t *conn_to   = memory_alloc(sizeof(region_connector_t), MEM_TAG_TEMP);
                        conn_to->next   = NULL;
                        conn_to->region = from;
                        conn_to->p      = (vec2i_t){col,row};
                        region_connectors[to - 1] = append_connector_to_tail(region_connectors[to - 1], conn_to);
                        break;
                    }
                }
            }
        }
    }

    //now we have linked lists for each room. 
    //Once a connector has been found, print the connection and remove all other connections between these two rooms
    for (uint32_t i = 0; i < grid->region_count; i++)
    {
        region_connector_t *head = region_connectors[i];
        while (head)
        {
            uint8_t to   = head->region;
            uint8_t from = (uint8_t)(i + 1);

            //make this point a floor
            grid_set(grid, head->p.y, head->p.x, (grid_tile_t){FLOOR, from});
            //delete from->to
            while (delete_connector(&head, to)){}

            //delete to->from too
            region_connector_t *to_head = region_connectors[to - 1];
            while(delete_connector(&to_head, from)) {}
        }
    }
}

void remove_dead_ends(grid_t *grid)
{
    bool done = false;

    while(!done)
    {
        done = true;

        for (uint32_t row = 1; row < grid->height - 1; row++)
        {
            for (uint32_t col = 1; col < grid->width - 1; col++)
            {
                if (grid_get_tile(grid, row,col) == WALL) continue;
                
                uint32_t exits = 0;
                for (uint32_t i = 0; i < 4; i++)
                {
                    if (grid_get_tile(grid, row + cardinal[i].y, col + cardinal[i].x) != WALL) exits++;
                }
                if (exits != 1) continue;

                done = false;
                grid_set_tile(grid, row, col, WALL);
            }
        }
    }
}

void create_dungeon(void)
{
    uint32_t max_tries = 250;

    float map_width = (float)GRID_SIZE;
    float map_height = (float)GRID_SIZE;

    uint32_t room_count = 0;
    rect_t rooms[ROOM_COUNT];

    grid_t grid;
    grid_init(&grid, GRID_SIZE, GRID_SIZE);

    grid.region_count++;

    rect_t *first_room = &rooms[room_count++];
    first_room->min.x = 1;
    first_room->min.y = 1;
    first_room->size.x = 11;
    first_room->size.y = 11;

    for (uint32_t row = 1; row < 12; row++)
    {
        for (uint32_t col = 1; col < 12; col++)
        {
            grid_set_tile(&grid, row, col, FLOOR);
            grid_set_region(&grid, row,col,grid.region_count);
        }
    }

    for (uint32_t i = 0; i < max_tries; i++)
    {
        //room size needs to be odd according to bob nystrom:
        //https://github.com/munificent/hauberk/blob/db360d9efa714efb6d937c31953ef849c7394a39/lib/src/content/dungeon.dart
        int width = (int)rng_in_range(9.0f, 18.0f);
        int height = (int)rng_in_range(9.0f, 18.0f);

        if (width % 2 == 0) width++;
        if (height % 2 == 0) height++;

        int x = (int)rng_in_range(0, map_width - width);
        int y = (int)rng_in_range(0, map_height - height);

        if (x % 2 == 0) x++;
        if (y % 2 == 0) y++;

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
        //new room means new region
        grid.region_count++;

        for (uint32_t row = y; row < y + height; row++)
        {
            for (uint32_t col = x; col < x + width; col++)
            {
                grid_set_tile(&grid, row, col, FLOOR);
                grid_set_region(&grid, row, col, grid.region_count);
            }
        }
        if (room_count == ROOM_COUNT) break;
    }


    //now carve mazes
    for (uint32_t row = 1; row < grid.height; row += 2)
    {
        for (uint32_t col = 1; col < grid.width; col += 2)
        {
            if (grid_get_tile(&grid, row, col) == WALL)
            {
                grow_maze(&grid, col, row);
            }
        }
    }

    //for every point in the map, have a list of regions it connects
    connect_regions(&grid);
    //remove dead ends
    remove_dead_ends(&grid);

    //now write to file
    FILE *test_map = fopen("./assets/tilemaps/dungeon.map", "w");
    char *buf = memory_alloc(1024*1024, MEM_TAG_TEMP);
    char *ptr = buf;
    for (uint32_t row = 0; row  < GRID_SIZE; row++)
    {
        for (uint32_t col = 0; col < GRID_SIZE; col++)
        {
            int val = 11;
            if (grid_get_tile(&grid, row,col) == FLOOR) val = 140;
            ptr += sprintf(ptr, "%03d,",val);
        }
        ptr += sprintf(ptr, "\n");
    }
    fwrite(buf, 1, (size_t)(ptr - buf), test_map);
    fclose(test_map);

    ptr = buf;
    memset(buf, 0, 1024 * 1024);

    //visualisation
    for (uint32_t row = 0; row  < GRID_SIZE; row++)
    {
        for (uint32_t col = 0; col < GRID_SIZE; col++)
        {
            ptr += sprintf(ptr, "%03d,",grid_get_region(&grid, row, col));
        }
        ptr += sprintf(ptr, "\n");
    }

    test_map = fopen("./assets/tilemaps/test.map", "w");
    fwrite(buf, 1, (size_t)(ptr - buf), test_map);
    fclose(test_map);

    LOGI("SUCCESS");
}
