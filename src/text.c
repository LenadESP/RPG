#include "text.h"
#include "canvas.h"
#include "defines.h"

#include "raylib.h"
#include <stdlib.h>

//Alagard, with the accents added by tools/font/add_accents.py (the original has none)
static const char *font_path = "assets/Ui/Font/AlagardES.ttf";

//Raylib only loads the letters in this list. Anything else comes out as '?'
static const char *font_characters =
    " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"
    "áéíóúüñÁÉÍÓÚÜÑ¿¡";

//If there's no translation, leave it out and the spanish one is used
static const char *text_defs[TXT_COUNT][LANG_COUNT] = {
    [TXT_PLAY]    = {[LANG_ES] = "Jugar",           [LANG_EN] = "Play",          [LANG_EU] = "Jolastu"},
    [TXT_CONFIG]  = {[LANG_ES] = "Configuración",   [LANG_EN] = "Configuration", [LANG_EU] = "Konfigurazioa"},
    [TXT_EXIT]    = {[LANG_ES] = "Salir del juego", [LANG_EN] = "Close game",    [LANG_EU] = "Itxi jokua"},
    [TXT_RESUME]  = {[LANG_ES] = "Volver al juego", [LANG_EN] = "Resume",        [LANG_EU] = "Jokura itzuli"},
    [TXT_TO_MENU] = {[LANG_ES] = "Salir al menú",   [LANG_EN] = "Exit to menu",  [LANG_EU] = "Menura itzuli"},
};

//Same colors as the old text sprites
static const Color text_color    = {251, 228, 168, 255};
static const Color outline_color = { 29,   5,  28, 255};

static Font font;
static Language current_language = LANG_ES;

void font_load(void){
    int character_count = 0;
    int *characters = LoadCodepoints(font_characters, &character_count);
    font = LoadFontEx(font_path, FONT_LOAD_SIZE, characters, character_count);
    UnloadCodepoints(characters);

    //If it fails raylib draws with its own font, so it's just a warning
    if(!IsFontValid(font)){
        TraceLog(LOG_WARNING, "ERROR! Asset %s could not load properly. Please check game integrity.", font_path);
    }

    //Loaded big and scaled down, like the menu textures
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
}

void font_unload(void){
    UnloadFont(font);
}

void text_set_language(Language language){
    current_language = language;
}

const char *text_get(TextId id){
    const char *text = text_defs[id][current_language];

    if(text == NULL){
        text = text_defs[id][LANG_ES];
    }

    return text;
}

void text_draw(TextId id, Rectangle area, float scale, float offset_x, float offset_y){
    const char *text = text_get(id);
    float font_size = MENU_TEXT_HEIGHT * scale;
    Vector2 text_size = MeasureTextEx(font, text, font_size, 0);
    Rectangle screen_area = canvas_to_screen(area, scale, offset_x, offset_y);

    //Centered in the area
    Vector2 position = {screen_area.x + (screen_area.width  - text_size.x) / 2.0f,
                        screen_area.y + (screen_area.height - text_size.y) / 2.0f};

    //Outline: the text 9 times around itself in the dark color, then the real one on top
    float outline_width = TEXT_OUTLINE_WIDTH * scale;

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            Vector2 outline_position = {position.x + dx * outline_width, position.y + dy * outline_width};
            DrawTextEx(font, text, outline_position, font_size, 0, outline_color);
        }
    }

    DrawTextEx(font, text, position, font_size, 0, text_color);
}
