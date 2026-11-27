#include "input.h"
#include "settings.h"
#include "raylib.h"

void input_poll_player(PlayerInput *player_input)
{
    player_input->up = IsKeyDown(settings_get(SETTING_KEY_UP));
    player_input->down = IsKeyDown(settings_get(SETTING_KEY_DOWN));
    player_input->left = IsKeyDown(settings_get(SETTING_KEY_LEFT));
    player_input->right = IsKeyDown(settings_get(SETTING_KEY_RIGHT));
}

void input_poll_ui(UIInput *ui_input){
    ui_input->fullscreen = IsKeyPressed(settings_get(SETTING_KEY_FULLSCREEN));
    ui_input->debug_mode = IsKeyPressed(KEY_F3); //Not in the settings on purpose
    ui_input->skip_loading = IsKeyPressed(settings_get(SETTING_KEY_SKIP_LOADING));
    ui_input->pause_game = IsKeyPressed(settings_get(SETTING_KEY_PAUSE_GAME));
}
