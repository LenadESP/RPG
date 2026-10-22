#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>

typedef struct Entity {
    float x;
    float y;
    float size;
    float vel;
    float health;
    float max_health;
    int texture_id;
    bool active;
} Entity;

#endif