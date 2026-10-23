#include "input.h"
#include "raylib.h"

void input_poll_player(PlayerInput *player_input)
{
    player_input->up = IsKeyDown(KEY_W);
    player_input->down = IsKeyDown(KEY_S);
    player_input->left = IsKeyDown(KEY_A);
    player_input->right = IsKeyDown(KEY_D);
}

void input_poll_ui(UIInput *ui_input){
    ui_input->fullscreen = IsKeyPressed(KEY_F11);
    ui_input->start_game = IsKeyPressed(KEY_ENTER);
    ui_input->pause_game = IsKeyPressed(KEY_ESCAPE);
}