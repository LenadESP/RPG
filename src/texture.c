#include "texture.h"
#include <stdbool.h>

typedef struct {
    const char *path;
    int frame_w;
    int frame_h;
    int off_x;
    int off_y;
    bool smooth;
} TextureDef;


//This is basically the list of all textures avaiable. 
//TEX NONE MUST ALWAYS REMAIN THE FIRST
static const TextureDef texture_defs[TEX_COUNT] = {
    [TEX_NONE]   = { "assets/none.png", 16, 16, 8, 16},
    [TEX_PLAYER] = { "assets/Actor/CharacterAnimated/NinjaGreen/SpriteSheet.png", 32, 32, 16, 24, false},
    [TEX_TILESET_FLOOR] = {"assets/Backgrounds/Tilesets/TilesetFloor.png", 16, 16, 0, 0},
    [TEX_TILESET_FLOOR_DETAIL] = {"assets/Backgrounds/Tilesets/TilesetFloorDetail.png", 16, 16, 0, 0, false},
    [TEX_TILESET_ELEMENT] = {"assets/Backgrounds/Tilesets/TilesetElement.png", 16, 16, 0, 0, false},
    [TEX_TILESET_NATURE] = {"assets/Backgrounds/Tilesets/TilesetNature.png", 16, 16, 0, 0, false},
    [TEX_TILESET_COLLISIONS] = {"assets/Backgrounds/Tilesets/TilesetLogic.png", 16, 16, 0, 0, false},
    
    [TEX_MAINMENU_BG]     = {"assets/Menus/MainMenu/sheets/background.png", 1672, 941, 0, 0, true},
    [TEX_MAINMENU_LOGO]   = {"assets/Menus/MainMenu/sheets/logo.png", 1443, 935, 0, 0, true},
    [TEX_MAINMENU_NBUTTON] = {"assets/Menus/MainMenu/sheets/boton_normal.png", 1340, 258, 0, 0, true},
    [TEX_MAINMENU_HBUTTON] = {"assets/Menus/MainMenu/sheets/boton_hover.png", 1244, 242, 0, 0, true},

        [TEX_MAINMENU_TEXT_CONFIGURACION] = {"assets/Menus/MainMenu/split/textos_configuracion_es.png", 889, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_CONFIGURATION] = {"assets/Menus/MainMenu/split/textos_configuracion_en.png", 858, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_KONFIGURAZIOA] = {"assets/Menus/MainMenu/split/textos_configuracion_eu.png", 884, 168, 0, 0, true},

    [TEX_MAINMENU_TEXT_JUGAR]   = {"assets/Menus/MainMenu/split/textos_jugar_play_jolastu_es.png", 419, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_PLAY]    = {"assets/Menus/MainMenu/split/textos_jugar_play_jolastu_en.png", 309, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_JOLASTU] = {"assets/Menus/MainMenu/split/textos_jugar_play_jolastu_eu.png", 502, 168, 0, 0, true},

    [TEX_MAINMENU_TEXT_SALIR] = {"assets/Menus/MainMenu/split/textos_salir_es.png", 925, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_EXIT]  = {"assets/Menus/MainMenu/split/textos_salir_en.png", 741, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_ITXI]  = {"assets/Menus/MainMenu/split/textos_salir_eu.png", 680, 168, 0, 0, true},

    [TEX_MAINMENU_TEXT_VOLVER_AL_JUEGO] = {"assets/Menus/MainMenu/split/textos_volver_al_juego_es.png", 809, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_RESUME]          = {"assets/Menus/MainMenu/split/textos_resume_en.png", 490, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_ITZULI_JOKURA]   = {"assets/Menus/MainMenu/split/textos_jokura_itzuli_eu.png", 770, 168, 0, 0, true},

    [TEX_MAINMENU_TEXT_VOLVER_AL_MENU] = {"assets/Menus/MainMenu/split/textos_salir_al_menu_es.png", 765, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_MENU]           = {"assets/Menus/MainMenu/split/textos_exit_to_menu_en.png", 742, 168, 0, 0, true},
    [TEX_MAINMENU_TEXT_ITZULI_MENURA]  = {"assets/Menus/MainMenu/split/textos_menura_itzuli_eu.png", 758, 168, 0, 0, true},
};

static TextureAsset textures[TEX_COUNT];

//this just loads the previous shi)
void texture_load_all(void){
    for (int i = 0; i < TEX_COUNT; i++) {

        textures[i].texture = LoadTexture(texture_defs[i].path);
        //Texture integrity logic

        if(textures[i].texture.id == 0){
            if(i == TEX_NONE){
                TraceLog(LOG_FATAL, "ERROR! CHECK GAME INTEGRITY. %s NOT FOUND. THIS IS CORE TO THE GAME'S FUNCTIONALITY, EXITING NOW.", texture_defs[i].path);
            } else {
                TraceLog(LOG_WARNING, "ERROR! Asset %s could not load properly. Please check game integrity.", texture_defs[i].path);
                textures[i] = textures[TEX_NONE];
            }
        } else {
            //Set mode for bigger ahh textures so they don't look weird.
            if (texture_defs[i].smooth) {
                SetTextureFilter(textures[i].texture, TEXTURE_FILTER_BILINEAR);
            }

            textures[i].frame_w = texture_defs[i].frame_w;
            textures[i].frame_h = texture_defs[i].frame_h;
            textures[i].off_x = texture_defs[i].off_x;
            textures[i].off_y = texture_defs[i].off_y;
            textures[i].columns = textures[i].texture.width  / texture_defs[i].frame_w;
            textures[i].rows    = textures[i].texture.height / texture_defs[i].frame_h;
        }
    }
}

void texture_unload_all(void){
    for (int i = 0; i < TEX_COUNT; i++) {
        UnloadTexture(textures[i].texture);
    }
}

TextureAsset texture_get(TextureId id){
    return textures[id];
}