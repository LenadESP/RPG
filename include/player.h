#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "input.h"
#include "entity.h"

Entity player_init(void);
void player_update (Entity *p, InputState i, double tick_duration);

#endif