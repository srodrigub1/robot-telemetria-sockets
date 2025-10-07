#ifndef WORLD_MODULE_H
#define WORLD_MODULE_H
#include <stdbool.h>

#define WORLD_W 10
#define WORLD_H 10

typedef enum { WORLD_OK=0, WORLD_OBSTACLE, WORLD_OOB, WORLD_INVALID_DIR } world_status_t;
typedef enum { DIR_LEFT=0, DIR_RIGHT, DIR_UP, DIR_DOWN } world_dir_t;

void world_init(void);
void world_get_pos(int *x, int *y);
bool world_parse_dir(const char *s, world_dir_t *d);
world_status_t world_move(world_dir_t dir, int steps, int *ox, int *oy);

#endif
