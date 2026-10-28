#include "player.h"
#include "entity.h"
#include "input.h"
#include "texture.h"

#include <math.h>

#define MAX_HEALTH 100
#define START_HEALTH 100
#define VEL 200
#define HB_W 8
#define HB_H 12
#define START_X 400
#define START_Y 225


Entity player_init(void){
    Entity player = {
        .x = START_X,
        .y = START_Y,
        .hb_w = HB_W,
        .hb_h = HB_H,
        .vel = VEL,
        .health = START_HEALTH,
        .max_health = MAX_HEALTH,
        .texture_id = TEX_PLAYER,
        .active = true,
        .solid = true
    };

    return player;
}

void player_update (Entity *p, PlayerInput i, float tick_duration, Entity *entities, int count){
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
        entity_update(p, dx/length, dy/length, tick_duration, entities, count);
    } else {
        entity_update(p, dx, dy, tick_duration, entities, count);
    }
}