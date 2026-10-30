#include "camera.h"
#include "raylib.h"
#include "entity.h"

#include <math.h>

#define VIRTUAL_W 240   // 30 tiles de 16px
#define VIRTUAL_H 136   // 17 tiles de 16px

void camera_update(Camera2D *cam, const Entity *player){ 
    //ZOOM LOGIC
    int zoom_x = GetScreenWidth() / VIRTUAL_W;
    int zoom_y = GetScreenHeight() / VIRTUAL_H;
    int zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    if (zoom < 1) zoom = 1;
    
    //CAMERA LOGIC
    *cam = (Camera2D){
        (Vector2){ floorf(GetScreenWidth() / 2.0f), floorf(GetScreenHeight() / 2.0f) }, //offset
        (Vector2) {player->x , player->y - player->hb_h / 2.0f}, //middle of player
        0.0f, //Rotation
        (float)zoom
    };

}