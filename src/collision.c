#include "entity.h"
#include "raylib.h"

Rectangle collision_rect(const Entity *e) {
    return (Rectangle){ e->x, e->y, e->size, e->size };
}