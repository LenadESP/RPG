#include "input.h"
#include "raylib.h"

void input_poll(InputState *input)
{
    input->up = IsKeyDown(KEY_W);
    input->down = IsKeyDown(KEY_S);
    input->left = IsKeyDown(KEY_A);
    input->right = IsKeyDown(KEY_D);
}