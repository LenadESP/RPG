#include "raylib.h"

Rectangle canvas_to_screen(Rectangle area, float scale, float offset_x, float offset_y)
{
    return (Rectangle){
        offset_x + area.x * scale,
        offset_y + area.y * scale,
        area.width  * scale,
        area.height * scale
    };
}