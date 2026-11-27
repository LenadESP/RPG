#include "input.h"
#include "entity.h"
#include "player.h"
#include "game.h"
#include "raylib.h"
#include "camera.h"
#include "texture.h"
#include "map.h"
#include "sound.h"
#include "text.h"
#include "settings.h"
#include "canvas.h"

#include "defines.h"

#include <stdlib.h>
#include <stdbool.h>

static void game_shutdown(void);

//state functions
static void set_playing(void);
static void set_paused(void);
static void set_menu(void);
static void open_settings(void);
static void close_settings(void);
static void close_game(void);

//NOTICE: close game and game shutdown are NOT the same. close game just sends the closing signal, and game shutdown handles the actual logic.

static double accumulator = 0.0;
static double lastTime = 0.0;
static bool shouldCloseGame = false;

typedef enum {
        GAME_MENU,      
        GAME_PLAYING,   
        GAME_PAUSED,
        GAME_LOADING,
        GAME_SETTINGS
} GameState;

//Set the initial screen state to be the menu
static GameState game_state = GAME_LOADING;

//Settings opens from the menu or the pause, and closing goes back there
static GameState state_before_settings = GAME_MENU;

int main(void) {
    //Initialize some variables
    PlayerInput player_input = {0};
    UIInput ui_input = {0};
    Map map = {0};

    const LayerDef camino_sur_layers[] = {
        { "assets/Maps/camino_sur/camino_sur_Ground.csv", TEX_TILESET_FLOOR        },
        { "assets/Maps/camino_sur/camino_sur_Decor.csv",  TEX_TILESET_FLOOR_DETAIL },
        { "assets/Maps/camino_sur/camino_sur_Props.csv",  TEX_TILESET_ELEMENT  },
        { "assets/Maps/camino_sur/camino_sur_Trees.csv",  TEX_TILESET_NATURE  },
        { "assets/Maps/camino_sur/camino_sur_AbovePlayer.csv", TEX_TILESET_NATURE},
    };

    const char *collision_path = "assets/Maps/camino_sur/camino_sur_Collision.csv";

    bool debug = false;

    //Define entities array
    Entity entities[MAX_ENTITIES] = {0};

    //Initialize and create player alias
    entities[0] = player_init();
    Entity *player = &entities[0];

    //Accumulator
    const float TICK_DURATION = 1.0 / TICK_RATE;

    //camera logic
    Camera2D camera = {0};

    //Start window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_MAXIMIZED);
    InitWindow(1280, 720, "RPG Game!"); //16:9
    InitAudioDevice();
    SetExitKey(KEY_NULL);
    SetTargetFPS(FRAME_RATE);

    //SET ATEXIT: Basically, set what exit() calls.
    atexit(game_shutdown);

    //Load all textures. This is where a loading screen would come in handy.
    texture_load_all();
    sound_load_all();
    font_load();
    settings_init();
    map_load(&map, camino_sur_layers, sizeof(camino_sur_layers) / sizeof(camino_sur_layers[0]), 4, collision_path);

    //Calculate map height and width in pixels
    int map_h = map.h * TILE_SIZE;
    int map_w = map.w * TILE_SIZE;

    //Reset timer
    lastTime = GetTime();

    while (!WindowShouldClose() && !shouldCloseGame) {
        input_poll_player(&player_input);
        input_poll_ui(&ui_input);

        //While a key is being rebound, every key belongs to that
        if (widget_waiting_for_key()) {
            ui_input = (UIInput){0};
        }

        //fullscreen handler
        if (ui_input.fullscreen){settings_toggle_fullscreen();}

        //Activate debug mode
        if (ui_input.debug_mode){debug = !debug;}

        if(ui_input.skip_loading && game_state == GAME_LOADING){
            set_menu();
        }

        //Game pause
        if (ui_input.pause_game && game_state == GAME_PLAYING){
            set_paused();
        
        } else if (ui_input.pause_game && game_state == GAME_PAUSED){
            set_playing();

        } else if (ui_input.pause_game && game_state == GAME_SETTINGS){
            close_settings();
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
                player_update(player, player_input, TICK_DURATION, entities, MAX_ENTITIES, &map);
                accumulator -= TICK_DURATION;
            }
        }

        //DRAWING LOGIC
        //Careful with the order: It's layered

        BeginDrawing();
        ClearBackground(BLACK);
        
        //Update camera
        camera_update(&camera, player, map_w, map_h);

        switch (game_state) {
            case GAME_MENU: {
                game_menu_draw(set_playing, open_settings, close_game);
                break;
            }

            case GAME_PLAYING: {
                game_draw(entities, MAX_ENTITIES, debug, &camera, &map);
                break;
            }
            
            case GAME_PAUSED: {
                game_pause_draw(entities, MAX_ENTITIES, debug, &camera, &map, set_playing, open_settings, set_menu);
                break;
            }
            case GAME_LOADING: {
                game_load_draw(set_menu);
                break;
            }
            case GAME_SETTINGS: {
                bool opened_from_pause = (state_before_settings == GAME_PAUSED);
                game_settings_draw(entities, MAX_ENTITIES, debug, &camera, &map, opened_from_pause, close_settings);
                break;
            }
        }   

        //player position for spawn point lmfao
        DrawText(TextFormat("X: %.2f", entities[0].x), 10, 10, 20, BLACK);
        DrawText(TextFormat("Y: %.2f", entities[0].y), 10, 34, 20, BLACK);

        //Brightness: a black veil over everything
        float brightness = MIN_BRIGHTNESS + (1.0f - MIN_BRIGHTNESS) * settings_get(SETTING_BRIGHTNESS) / 100.0f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 1.0f - brightness));

        EndDrawing();
    }
    map_unload(&map);
    //also calls exit()
    return 0;
}

void game_shutdown(void){
    texture_unload_all();
    sound_unload_all();
    font_unload();

    CloseAudioDevice();
    CloseWindow();
}

void close_game(void){
    shouldCloseGame = true;
}

void set_playing(void){
    lastTime = GetTime();
    accumulator = 0;
    game_state = GAME_PLAYING;
}
void set_paused(void){
    lastTime = GetTime();
    accumulator = 0;
    game_state = GAME_PAUSED;
}
void set_menu(void){
    lastTime = GetTime();
    accumulator = 0;
    game_state = GAME_MENU;
}
void open_settings(void){
    //Whatever was left unapplied last time is thrown away
    settings_discard();
    state_before_settings = game_state;
    game_state = GAME_SETTINGS;
}
void close_settings(void){
    game_state = state_before_settings;
}
