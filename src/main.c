#include "input.h"
#include "raylib.h"
#include <stdio.h>

int main(void) {
    InputState input = {0};

    InitWindow(800, 450, "test");
    while (!WindowShouldClose()) {
        input_poll(&input);
        if (input.up) {
            printf("Up key is pressed\n");
        }
        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("si lees esto, funciona", 190, 200, 20, RAYWHITE);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}