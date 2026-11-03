#include "camera.h"
#include "raylib.h"
#include "entity.h"

#include <math.h>

#define VIRTUAL_W 240   // 30 tiles de 16px
#define VIRTUAL_H 136   // 17 tiles de 16px

void camera_update(Camera2D *cam, const Entity *player, int map_w, int map_h){ 
    //ZOOM LOGIC
    int zoom_x = GetScreenWidth() / VIRTUAL_W;
    int zoom_y = GetScreenHeight() / VIRTUAL_H;
    int zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    if (zoom < 1) zoom = 1;

    //Calculate medium viewport
    float medium_y = (GetScreenHeight()/2.0f) / zoom;
    float medium_x = (GetScreenWidth()/2.0f) / zoom;


    //Set and clamp target
    float target_x = player->x;
    float target_y = player->y - player->hb_h / 2.0f;

    if(target_x < medium_x){target_x = medium_x;}
    if(target_x > map_w - medium_x){target_x = map_w - medium_x;}
    
    if(target_y < medium_y){target_y = medium_y;}
    if(target_y > map_h - medium_y){target_y = map_h - medium_y;}

    //idk what ts does but it fixes stripping so it must me important ig.
    target_x = floorf(target_x * zoom) / zoom;
    target_y = floorf(target_y * zoom) / zoom;
    
    //CAMERA LOGIC
    *cam = (Camera2D){
        (Vector2){ floorf(GetScreenWidth() / 2.0f), floorf(GetScreenHeight() / 2.0f) }, //offset
        (Vector2) {target_x, target_y}, //middle of player
        0.0f, //Rotation
        (float)zoom
    };

}