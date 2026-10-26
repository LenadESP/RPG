#ifndef SCREEN_H
#define SCREEN_H

#include "entity.h"

void screen_menu_draw(void);
void screen_pause_draw(const Entity *entities, int count, bool debug);
void screen_game_draw(const Entity *entities, int count, bool debug);

#endif