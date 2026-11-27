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

    //Pixel art drawn at 2x, so no smoothing
    [TEX_SETTINGS_PANEL]              = {"assets/Menus/Settings/panel.png", 686, 342, 0, 0, false},
    [TEX_SETTINGS_TITLE]              = {"assets/Menus/Settings/title_ornate.png", 380, 48, 0, 0, false},
    [TEX_SETTINGS_TAB_NORMAL]         = {"assets/Menus/Settings/tab_normal.png", 152, 32, 0, 0, false},
    [TEX_SETTINGS_TAB_HOVER]          = {"assets/Menus/Settings/tab_hover.png", 152, 32, 0, 0, false},
    [TEX_SETTINGS_ACTION_NORMAL]      = {"assets/Menus/Settings/action_normal.png", 194, 35, 0, 0, false},
    [TEX_SETTINGS_ACTION_HOVER]       = {"assets/Menus/Settings/action_hover.png", 194, 35, 0, 0, false},
    [TEX_SETTINGS_SEPARATOR]          = {"assets/Menus/Settings/separator.png", 434, 5, 0, 0, false},
    [TEX_SETTINGS_SEPARATOR_VERTICAL] = {"assets/Menus/Settings/separator_vertical.png", 5, 224, 0, 0, false},
    [TEX_SETTINGS_DROPDOWN_NORMAL]    = {"assets/Menus/Settings/dropdown_normal.png", 192, 27, 0, 0, false},
    [TEX_SETTINGS_DROPDOWN_HOVER]     = {"assets/Menus/Settings/dropdown_hover.png", 192, 27, 0, 0, false},
    [TEX_SETTINGS_DROPDOWN_LIST]      = {"assets/Menus/Settings/dropdown_list.png", 192, 80, 0, 0, false},
    [TEX_SETTINGS_OPTION_NORMAL]      = {"assets/Menus/Settings/option_normal.png", 186, 24, 0, 0, false},
    [TEX_SETTINGS_OPTION_HOVER]       = {"assets/Menus/Settings/option_hover.png", 186, 24, 0, 0, false},
    [TEX_SETTINGS_KEYCAP_NORMAL]      = {"assets/Menus/Settings/keycap_normal.png", 65, 24, 0, 0, false},
    [TEX_SETTINGS_KEYCAP_HOVER]       = {"assets/Menus/Settings/keycap_hover.png", 65, 24, 0, 0, false},
    [TEX_SETTINGS_SLIDER_TRACK]       = {"assets/Menus/Settings/slider_track.png", 204, 9, 0, 0, false},
    [TEX_SETTINGS_SLIDER_FILL]        = {"assets/Menus/Settings/slider_fill.png", 198, 3, 0, 0, false},
    [TEX_SETTINGS_SLIDER_THUMB]       = {"assets/Menus/Settings/slider_thumb.png", 15, 17, 0, 0, false},
    [TEX_SETTINGS_CHECKBOX_OFF]       = {"assets/Menus/Settings/checkbox_off.png", 20, 20, 0, 0, false},
    [TEX_SETTINGS_CHECKBOX_ON]        = {"assets/Menus/Settings/checkbox_on.png", 20, 20, 0, 0, false},
    [TEX_SETTINGS_RADIO_OFF]          = {"assets/Menus/Settings/radio_off.png", 17, 17, 0, 0, false},
    [TEX_SETTINGS_RADIO_ON]           = {"assets/Menus/Settings/radio_on.png", 17, 17, 0, 0, false},
    [TEX_SETTINGS_ARROW_DOWN]         = {"assets/Menus/Settings/arrow_down.png", 11, 11, 0, 0, false},
    [TEX_SETTINGS_ARROW_UP]           = {"assets/Menus/Settings/arrow_up.png", 11, 11, 0, 0, false},
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