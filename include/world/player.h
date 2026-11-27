#ifndef PLAYER_H
#define PLAYER_H

#include "input.h"
#include "entity.h"
#include "map.h"

#include <stdbool.h>

Entity player_init(void);
void player_update (Entity *p, PlayerInput i, float tick_duration, Entity *entities, int count, Map *map);

#endif