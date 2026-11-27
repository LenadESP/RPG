#include "text.h"
#include "canvas.h"
#include "defines.h"
#include "colors.h"

#include "raylib.h"
#include <stdlib.h>
#include <stdbool.h>

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

    [TXT_GENERAL]       = {[LANG_ES] = "General",             [LANG_EN] = "General",        [LANG_EU] = "Orokorra"},
    [TXT_VIDEO]         = {[LANG_ES] = "Vídeo",               [LANG_EN] = "Video",          [LANG_EU] = "Bideoa"},
    [TXT_AUDIO]         = {[LANG_ES] = "Audio",               [LANG_EN] = "Audio",          [LANG_EU] = "Audioa"},
    [TXT_CONTROLS]      = {[LANG_ES] = "Controles",           [LANG_EN] = "Controls",       [LANG_EU] = "Kontrolak"},
    [TXT_LANGUAGE]      = {[LANG_ES] = "Idioma",              [LANG_EN] = "Language",       [LANG_EU] = "Hizkuntza"},
    [TXT_APPLY]         = {[LANG_ES] = "Aplicar",             [LANG_EN] = "Apply",          [LANG_EU] = "Aplikatu"},
    [TXT_CANCEL]        = {[LANG_ES] = "Cancelar",            [LANG_EN] = "Cancel",         [LANG_EU] = "Utzi"},
    [TXT_RESET]         = {[LANG_ES] = "Restablecer",         [LANG_EN] = "Reset defaults", [LANG_EU] = "Lehenetsi"},
    [TXT_SHOW_HINTS]    = {[LANG_ES] = "Mostrar ayudas",      [LANG_EN] = "Show hints",     [LANG_EU] = "Laguntzak erakutsi"},
    [TXT_YES]           = {[LANG_ES] = "Sí",                  [LANG_EN] = "On",             [LANG_EU] = "Bai"},
    [TXT_NO]            = {[LANG_ES] = "No",                  [LANG_EN] = "Off",            [LANG_EU] = "Ez"},
    [TXT_TEXT_SPEED]    = {[LANG_ES] = "Velocidad del texto", [LANG_EN] = "Text speed",     [LANG_EU] = "Testuaren abiadura"},
    [TXT_SLOW]          = {[LANG_ES] = "Lenta",               [LANG_EN] = "Slow",           [LANG_EU] = "Motela"},
    [TXT_NORMAL]        = {[LANG_ES] = "Normal",              [LANG_EN] = "Normal",         [LANG_EU] = "Normala"},
    [TXT_FAST]          = {[LANG_ES] = "Rápida",              [LANG_EN] = "Fast",           [LANG_EU] = "Azkarra"},
    [TXT_DISPLAY_MODE]  = {[LANG_ES] = "Modo de pantalla",    [LANG_EN] = "Display mode",   [LANG_EU] = "Pantaila modua"},
    [TXT_WINDOWED]      = {[LANG_ES] = "Ventana",             [LANG_EN] = "Windowed",       [LANG_EU] = "Leihoa"},
    [TXT_FULLSCREEN]    = {[LANG_ES] = "Pantalla completa",   [LANG_EN] = "Fullscreen",     [LANG_EU] = "Pantaila osoa"},
    [TXT_BORDERLESS]    = {[LANG_ES] = "Sin bordes",          [LANG_EN] = "Borderless",     [LANG_EU] = "Ertzik gabe"},
    [TXT_BRIGHTNESS]    = {[LANG_ES] = "Brillo",              [LANG_EN] = "Brightness",     [LANG_EU] = "Distira"},
    [TXT_MASTER_VOLUME] = {[LANG_ES] = "Volumen general",     [LANG_EN] = "Master volume",  [LANG_EU] = "Bolumen orokorra"},
    [TXT_MUSIC]         = {[LANG_ES] = "Música",              [LANG_EN] = "Music",          [LANG_EU] = "Musika"},
    [TXT_SOUND_EFFECTS] = {[LANG_ES] = "Efectos de sonido",   [LANG_EN] = "Sound effects",  [LANG_EU] = "Soinu efektuak"},
    [TXT_INTERFACE]     = {[LANG_ES] = "Interfaz",            [LANG_EN] = "Interface",      [LANG_EU] = "Interfazea"},
    [TXT_AMBIENCE]      = {[LANG_ES] = "Ambiente",            [LANG_EN] = "Ambience",       [LANG_EU] = "Giroa"},
    [TXT_DIALOGUE]      = {[LANG_ES] = "Diálogos",            [LANG_EN] = "Dialogue",       [LANG_EU] = "Elkarrizketak"},
    [TXT_MOVEMENT]      = {[LANG_ES] = "Movimiento",          [LANG_EN] = "Movement",       [LANG_EU] = "Mugimendua"},
    [TXT_UP]            = {[LANG_ES] = "Arriba",              [LANG_EN] = "Up",             [LANG_EU] = "Gora"},
    [TXT_DOWN]          = {[LANG_ES] = "Abajo",               [LANG_EN] = "Down",           [LANG_EU] = "Behera"},
    [TXT_LEFT]          = {[LANG_ES] = "Izquierda",           [LANG_EN] = "Left",           [LANG_EU] = "Ezkerra"},
    [TXT_RIGHT]         = {[LANG_ES] = "Derecha",             [LANG_EN] = "Right",          [LANG_EU] = "Eskuinera"},
    [TXT_SKIP_LOADING]  = {[LANG_ES] = "Saltar carga",        [LANG_EN] = "Skip loading",   [LANG_EU] = "Karga saltatu"},
    [TXT_PAUSE_GAME]    = {[LANG_ES] = "Pausar juego",        [LANG_EN] = "Pause game",     [LANG_EU] = "Jokoa pausatu"},
    [TXT_PRESS_A_KEY]   = {[LANG_ES] = "Pulsa una tecla",     [LANG_EN] = "Press a key",    [LANG_EU] = "Sakatu tekla bat"},

    //Each language is written in itself, so only the ES slot
    [TXT_EUSKARA]    = {[LANG_ES] = "Euskara"},
    [TXT_CASTELLANO] = {[LANG_ES] = "Castellano"},
    [TXT_ENGLISH]    = {[LANG_ES] = "English"},
};

typedef struct {
    float font_size;
    bool centered;
    bool outline;
    Color color;
} TextStyleDef;

//Buttons get an outline, everything else a shadow. Not centered means left aligned
static const TextStyleDef text_style_defs[TEXT_STYLE_COUNT] = {
    [TEXT_BUTTON]   = {48.0f, true,  true,  COLOR_CREAM},
    [TEXT_TAB]      = {40.0f, true,  true,  COLOR_CREAM},
    [TEXT_HEADER]   = {40.0f, false, false, COLOR_CREAM},
    [TEXT_LABEL]    = {32.0f, false, false, COLOR_CREAM},
    [TEXT_VALUE]    = {32.0f, true,  false, COLOR_CREAM},
    [TEXT_MUTED]    = {32.0f, false, false, COLOR_FADED_BROWN},
    [TEXT_CONFLICT] = {32.0f, true,  false, COLOR_CLASH_RED},
};

//Used for both the outline and the shadow
static const Color dark_color = COLOR_DARK_PURPLE;

static Font font;
static Language current_language = LANG_EU;

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

void text_draw(const char *text, TextStyle style, Rectangle area, CanvasTransform transform){
    TextStyleDef def = text_style_defs[style];
    float font_size = def.font_size * transform.scale;
    Vector2 text_size = MeasureTextEx(font, text, font_size, 0);
    Rectangle screen_area = canvas_to_screen(area, transform);

    //Always centered vertically
    Vector2 position = {screen_area.x, screen_area.y + (screen_area.height - text_size.y) / 2.0f};

    if(def.centered){
        position.x += (screen_area.width - text_size.x) / 2.0f;
    }

    if(def.outline){
        //Outline: the text 8 times around itself in the dark color, then the real one on top
        float outline_width = TEXT_OUTLINE_WIDTH * transform.scale;

        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                //The middle one would be covered by the real text anyway
                if (dx == 0 && dy == 0) {
                    continue;
                }

                Vector2 outline_position = {position.x + dx * outline_width, position.y + dy * outline_width};
                DrawTextEx(font, text, outline_position, font_size, 0, dark_color);
            }
        }
    } else {
        float shadow_offset = TEXT_SHADOW_OFFSET * transform.scale;
        Vector2 shadow_position = {position.x + shadow_offset, position.y + shadow_offset};
        DrawTextEx(font, text, shadow_position, font_size, 0, dark_color);
    }

    DrawTextEx(font, text, position, font_size, 0, def.color);
}
