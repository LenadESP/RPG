#ifndef TEXTURE_H
#define TEXTURE_H

#include "raylib.h"
#include <stdbool.h>

typedef struct TextureAsset {
    Texture2D texture;
    int frame_w;
    int frame_h;
    int off_x;
    int off_y;
    int columns;
    int rows;
} TextureAsset;

typedef enum {
    TEX_NONE,
    TEX_PLAYER,
    TEX_TILESET_FLOOR,
    TEX_TILESET_FLOOR_DETAIL,
    TEX_TILESET_ELEMENT,
    TEX_TILESET_NATURE,
    TEX_TILESET_COLLISIONS,
    TEX_MAINMENU_BG,
    TEX_MAINMENU_LOGO,
    TEX_MAINMENU_NBUTTON, //normal button
    TEX_MAINMENU_HBUTTON, //hover button

    //MENUS TEXT
    TEX_MAINMENU_TEXT_CONFIGURACION,
    TEX_MAINMENU_TEXT_CONFIGURATION,
    TEX_MAINMENU_TEXT_KONFIGURAZIOA,
    TEX_MAINMENU_TEXT_JUGAR,
    TEX_MAINMENU_TEXT_PLAY,
    TEX_MAINMENU_TEXT_JOLASTU,
    TEX_MAINMENU_TEXT_SALIR,
    TEX_MAINMENU_TEXT_EXIT,
    TEX_MAINMENU_TEXT_ITXI,
    TEX_MAINMENU_TEXT_VOLVER_AL_JUEGO,
    TEX_MAINMENU_TEXT_RESUME,
    TEX_MAINMENU_TEXT_ITZULI_JOKURA,
    TEX_MAINMENU_TEXT_VOLVER_AL_MENU,
    TEX_MAINMENU_TEXT_MENU,
    TEX_MAINMENU_TEXT_ITZULI_MENURA,
    //
    
    TEX_COUNT
} TextureId;

void texture_load_all(void);
void texture_unload_all(void);
TextureAsset texture_get(TextureId id);

#endif