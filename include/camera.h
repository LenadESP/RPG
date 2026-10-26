#ifndef CAMERA_H
#define CAMERA_H

#include "raylib.h"
#include "entity.h"

void camera_update(Camera2D *cam, const Entity *player);

#endif