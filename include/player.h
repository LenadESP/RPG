#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "input.h"
#include "entity.h"

Entity player_init(void);
void player_update (Entity *p, PlayerInput i, float tick_duration, Entity *entities, int count);

#endif