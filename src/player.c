#include "player.h"
#include "entity.h"
#include "input.h"

#include <math.h>

#define MAX_HEALTH 100
#define START_HEALTH 100
#define VEL 400
#define SIZE 30
#define START_X 400
#define START_Y 225


Entity player_init(void){
    Entity player = {
        .x = START_X,
        .y = START_Y,
        .size = SIZE,
        .vel = VEL,
        .health = START_HEALTH,
        .max_health = MAX_HEALTH,
        .active = true
    };

    return player;
}

void player_update (Entity *p, PlayerInput i, float tick_duration){
    float dx = 0;
    float dy = 0;

    if(i.up){
        dy = -1;
    } 
    if(i.down){
        dy = 1;
    }
    if(i.right){
        dx = 1;
    }
    if(i.left){
        dx = -1;
    }
    
    //Normalize diagonal
    const float length = sqrtf(dx*dx + dy*dy);

    if (length > 0){
        entity_update(p, dx/length, dy/length, tick_duration);
    } else {
        entity_update(p, dx, dy, tick_duration);
    }
}