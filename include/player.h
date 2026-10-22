#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include "input.h"
#include "input.c"

Entity player_init(void);
void player_udate (Entity *p, InputState *i, double tick_duration);

#endif