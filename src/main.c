#include "input.h"
#include "entity.h"
#include "player.h"
#include "screen.h"
#include "raylib.h"

#include <stdio.h>
#include <stdbool.h>

//Defining basic variables for the game.
#define TICK_RATE 60
#define FRAME_RATE 60
#define MAX_FRAMETIME 0.25
#define MAX_ENTITIES 256

static void toggle_fullscreen(void);
typedef enum {
        SCREEN_MENU,      
        SCREEN_PLAYING,   
        SCREEN_PAUSED
} ScreenState;


int main(void) {
    //Initialize some variables
    PlayerInput player_input = {0};
    UIInput ui_input = {0};

    //Define entities array
    Entity entities[MAX_ENTITIES] = {0};

    //Initialize and create player alias
    entities[0] = player_init();
    Entity *player = &entities[0];

    //Set the initial screen state to be the menu
    ScreenState screen = SCREEN_MENU;

    //placeholder test collision box
    entities[1] = (Entity){
        .x = 500,
        .y = 500,
        .size = 30,
        .active = true,
        .solid = true
    };

    //Accumulator
    double accumulator = 0.0;
    double lastTime = 0.0;
    const float TICK_DURATION = 1.0 / TICK_RATE;

    //Start window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(1280, 720, "RPG Game!"); //16:9
    SetExitKey(KEY_NULL);
    SetTargetFPS(FRAME_RATE);

    lastTime = GetTime();

    while (!WindowShouldClose()) {
        input_poll_player(&player_input);
        input_poll_ui(&ui_input);

        //fullscreen handler
        if (ui_input.fullscreen){toggle_fullscreen();}

        //Start game
        if (ui_input.start_game && screen == SCREEN_MENU){
            lastTime = GetTime();
            accumulator = 0;
            screen = SCREEN_PLAYING;
        }

        //Game pause
        if (ui_input.pause_game && screen == SCREEN_PLAYING){
            screen = SCREEN_PAUSED;
        
        } else if (ui_input.pause_game && screen == SCREEN_PAUSED){
            lastTime = GetTime();
            accumulator = 0;
            screen = SCREEN_PLAYING;
        }

        //Accumulator's logic
        double currentTime = GetTime();
        double delta = currentTime - lastTime;

        if (delta > MAX_FRAMETIME){
            delta = MAX_FRAMETIME;
        }

        accumulator += delta;
        lastTime = currentTime;

        if(screen == SCREEN_PLAYING){
            while (accumulator >= TICK_DURATION) {
                player_update(player, player_input, TICK_DURATION, entities, MAX_ENTITIES);
                accumulator -= TICK_DURATION;
            }
        }

        //DRAWING LOGIC
        //Careful with the order: It's layered

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (screen) {
            case SCREEN_MENU: {
                screen_menu_draw();
                break;
            }

            case SCREEN_PLAYING: {
                screen_game_draw(entities, MAX_ENTITIES);
                break;
            }
            
            case SCREEN_PAUSED: {
                screen_pause_draw(entities, MAX_ENTITIES);
                break;
            }
        }   

        EndDrawing();
    }
    CloseWindow();
    return 0;
}

//Fullscreen login function
static void toggle_fullscreen(void){
    static int past_w = 1280;
    static int past_h = 720;
    static bool is_fullscreen = false;

    if (!is_fullscreen){
        past_w = GetScreenWidth();
        past_h = GetScreenHeight();
        ToggleFullscreen();
        is_fullscreen = true;
    } else {
        ToggleFullscreen();
        is_fullscreen = false;
        SetWindowSize(past_w, past_h);
    }
}