#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

//x and y are the corner of the hb
typedef struct Entity {
    float x;
    float y;
    float hb_w;
    float hb_h;
    float vel;
    float health;
    float max_health;
    int texture_id;
    bool active;
    bool solid;
} Entity;

void entity_update(Entity *e, float dx, float dy, float dt,  Entity *entities, int count);
void entity_draw(const Entity *e);

#endif