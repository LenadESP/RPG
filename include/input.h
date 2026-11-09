#ifndef INPUT_H 
#define INPUT_H

#include <stdbool.h>

typedef struct PlayerInput {
    bool up;
    bool down;
    bool left;
    bool right;
} PlayerInput;

typedef struct UIInput {
    bool fullscreen;
    bool skip_loading;
    bool pause_game;
    bool debug_mode;
} UIInput;

void input_poll_player(PlayerInput *in);
void input_poll_ui(UIInput *in);

#endif // INPUT_H