#include "entity.h"
#include "raylib.h"
#include "collision.h"
#include <stdbool.h>

Rectangle collision_rect(const Entity *e) {
    return (Rectangle){e->x - e->hb_w/2, e->y - e->hb_h, e->hb_w, e->hb_h};
}

bool collision_check(const Entity *p, const Entity *e) {
    return CheckCollisionRecs(collision_rect(p), collision_rect(e));
}