#include "input.h"
#include "entity.h"
#include "player.h"
#include "screen.h"
#include "raylib.h"
#include "camera.h"

#include <stdio.h>
#include <stdbool.h>

//Defining basic variables for the game.
#define TICK_RATE 60
#define FRAME_RATE 60
#define MAX_FRAMETIME 0.25
#define MAX_ENTITIES 256

static void toggle_fullscreen(void);
typedef enum {
        GAME_MENU,      
        GAME_PLAYING,   
        GAME_PAUSED
} GameState;

int main(void) {
    //Initialize some variables
    PlayerInput player_input = {0};
    UIInput ui_input = {0};
    bool debug = false;

    //Define entities array
    Entity entities[MAX_ENTITIES] = {0};

    //Initialize and create player alias
    entities[0] = player_init();
    Entity *player = &entities[0];

    //Set the initial screen state to be the menu
    GameState game_state = GAME_MENU;

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

    //camera logic
    Camera2D camera = {0};

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

        //Activate debug mode
        if (ui_input.debug_mode){debug = !debug;}

        //Start game
        if (ui_input.start_game && game_state == GAME_MENU){
            lastTime = GetTime();
            accumulator = 0;
            game_state = GAME_PLAYING;
        }

        //Game pause
        if (ui_input.pause_game && game_state == GAME_PLAYING){
            game_state = GAME_PAUSED;
        
        } else if (ui_input.pause_game && game_state == GAME_PAUSED){
            lastTime = GetTime();
            accumulator = 0;
            game_state = GAME_PLAYING;
        }

        //Accumulator's logic
        double currentTime = GetTime();
        double delta = currentTime - lastTime;

        if (delta > MAX_FRAMETIME){
            delta = MAX_FRAMETIME;
        }

        accumulator += delta;
        lastTime = currentTime;

        if(game_state == GAME_PLAYING){
            while (accumulator >= TICK_DURATION) {
                player_update(player, player_input, TICK_DURATION, entities, MAX_ENTITIES);
                accumulator -= TICK_DURATION;
            }
        }

        //DRAWING LOGIC
        //Careful with the order: It's layered

        BeginDrawing();
        ClearBackground(RAYWHITE);
        
        //Update camera
        camera_update(&camera, player);

        switch (game_state) {
            case GAME_MENU: {
                game_menu_draw();
                break;
            }

            case GAME_PLAYING: {
                game_draw(entities, MAX_ENTITIES, debug, &camera);
                break;
            }
            
            case GAME_PAUSED: {
                game_pause_draw(entities, MAX_ENTITIES, debug, &camera);
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