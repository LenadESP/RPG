#include "entity.h"
#include "raylib.h"

void entity_update(Entity *e, float dx, float dy, float dt){
    e->x += dx * e->vel * dt;
    e->y += dy * e->vel * dt;
}

void entity_draw(const Entity e){
    DrawRectangle((int)e.x, (int)e.y, (int)e.size, (int)e.size, RED);
}