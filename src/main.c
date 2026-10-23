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

int main(void) {
    InputState input = {0};
    Entity player = player_init();

    //Accumulator
    double accumulator = 0.0;
    double lastTime = 0.0;
    const double TICK_DURATION = 1.0 / TICK_RATE;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(1280, 720, "RPG Game!"); //16:9
    SetTargetFPS(FRAME_RATE);

    lastTime = GetTime();

    while (!WindowShouldClose()) {
        //fullscreen handler
        if (IsKeyPressed(KEY_F11)){toggle_fullscreen();}

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

        while (accumulator >= TICK_DURATION) {
            player_update(&player, input, TICK_DURATION);
            accumulator -= TICK_DURATION;
        }

        //DRAWING LOGIC
        //Careful with the order: It's layered

        BeginDrawing();

        ClearBackground(BLACK);
        entity_draw(player);

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