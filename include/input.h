#ifndef INPUT_H 
#define INPUT_H

#include <stdbool.h>

typedef struct InputState {
    bool up;
    bool down;
    bool left;
    bool right;
} InputState;

void input_poll(InputState *in);

#endif // INPUT_H