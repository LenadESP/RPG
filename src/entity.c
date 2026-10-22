#include "entity.h"
#include <stdbool>

void entity_update(Entity *e, float dx, float dy, float dt){
    e->x += dx * e->vel * dt;
    e->y += dy * e->vel * dt;
}

void entity_draw(const Entity *e){

}