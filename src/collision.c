#include "entity.h"
#include "raylib.h"
#include "collision.h"
#include <stdbool.h>

Rectangle collision_rect(const Entity *e) {
    return (Rectangle){ e->x, e->y, e->size, e->size };
}

bool collision_check(const Entity *p, const Entity *e) {
    return CheckCollisionRecs(collision_rect(p), collision_rect(e));
}