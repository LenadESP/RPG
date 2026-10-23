#ifndef COLLISION_H
#define COLLISION_H

#include "entity.h"
#include "raylib.h"
#include <stdbool.h>

Rectangle collision_rect(const Entity *e);
bool collision_check(const Entity *p,const Entity *e);

#endif