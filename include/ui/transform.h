#ifndef TRANSFORM_H
#define TRANSFORM_H

//How the 1672x941 canvas lands on the window: scaled, then moved to the center
typedef struct CanvasTransform {
    float scale;
    float offset_x;
    float offset_y;
} CanvasTransform;

#endif
