#ifndef INPUT_H 
#define INPUT_H

#include "stdbool.h"

typedef struct InputState {
    bool up;
    bool down;
    bool left;
    bool right;
} InputState;

#endif // INPUT_H