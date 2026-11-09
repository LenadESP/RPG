#ifndef DEFINES_H
#define DEFINES_H

//Defining basic variables for the game. 
//TAKEN FROM ----- MAIN.C -----
#define TICK_RATE 60
#define FRAME_RATE 60
#define MAX_FRAMETIME 0.25
#define MAX_ENTITIES 256

//TAKEN FROM ----- PLAYER.C -----
#define MAX_HEALTH 100
#define START_HEALTH 100
#define PLAYER_VEL 130
#define HB_W 8
#define HB_H 12
#define START_X 327
#define START_Y 846

//TAKEN FROM ----- CAMERA.C -----
#define VIRTUAL_W 240
#define VIRTUAL_H 136

//TAKEN FROM ----- MAP.H -----
#define TILE_SIZE 16

//TAKEN FROM ----- GAME.C -----
#define CANVAS_WIDTH 1672
#define CANVAS_HEIGHT 941
#define MENU_TEXT_HEIGHT 66.0f

#define LOADING_DURATION 3.5f
#define LOADING_STRIPS 48
#define LOADING_MAX_OFFSET 140.0f
#define FADE_DURATION 0.35f

#endif