#include "input.h"
#include "entity.h"
#include "player.h"
#include "raylib.h"

#include <stdio.h>
#include <stdbool.h>

//Defining basic variables for the game.
#define TICK_RATE 60
#define FRAME_RATE 60
#define MAX_FRAMETIME 0.25

static void toggle_fullscreen(void);
typedef enum {
        SCREEN_MENU,      
        SCREEN_PLAYING,   
        SCREEN_PAUSED
} ScreenState;


int main(void) {
    //Initialize some variables
    InputState input = {0};
    Entity player = player_init();
    ScreenState screen = SCREEN_MENU;

    //Accumulator
    double accumulator = 0.0;
    double lastTime = 0.0;
    const double TICK_DURATION = 1.0 / TICK_RATE;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(1280, 720, "RPG Game!"); //16:9
    SetExitKey(KEY_NULL);
    SetTargetFPS(FRAME_RATE);

    lastTime = GetTime();

    while (!WindowShouldClose()) {
        //fullscreen handler
        if (IsKeyPressed(KEY_F11)){toggle_fullscreen();}

        //Game start
        if (IsKeyPressed(KEY_ENTER) && screen == SCREEN_MENU){
            lastTime = GetTime();
            accumulator = 0;
            screen = SCREEN_PLAYING;
        }

        //Game pause
        if (IsKeyPressed(KEY_ESCAPE) && screen == SCREEN_PLAYING){
            screen = SCREEN_PAUSED;
        
        } else if (IsKeyPressed(KEY_ESCAPE) && screen == SCREEN_PAUSED){
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

        //Input logic using accumulator
        input_poll(&input);

        if(screen == SCREEN_PLAYING){
            while (accumulator >= TICK_DURATION) {
                player_update(&player, input, TICK_DURATION);
                accumulator -= TICK_DURATION;
            }
        }

        //DRAWING LOGIC
        //Careful with the order: It's layered

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        switch (screen) {
            case SCREEN_MENU:
                DrawText("Press enter to start!", 500, 500, 15, BLACK);
                break;

            case SCREEN_PLAYING:
                entity_draw(player);
                break;
            
            case SCREEN_PAUSED:
                entity_draw(player);
                DrawText("Paused! Press ESC to resume", 500, 500, 15, BLACK);
                break;
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