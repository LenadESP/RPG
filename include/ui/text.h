#ifndef TEXT_H
#define TEXT_H

#include "raylib.h"

//If there's no translation, LANG_ES is the text
typedef enum {
    LANG_ES,
    LANG_EN,
    LANG_EU,
    LANG_COUNT
} Language;

typedef enum {
    TXT_PLAY,
    TXT_CONFIG,
    TXT_EXIT,
    TXT_RESUME,
    TXT_TO_MENU,

    //Settings menu
    TXT_GENERAL,
    TXT_VIDEO,
    TXT_AUDIO,
    TXT_CONTROLS,
    TXT_LANGUAGE,
    TXT_APPLY,
    TXT_CANCEL,
    TXT_RESET,
    TXT_SHOW_HINTS,
    TXT_YES,
    TXT_NO,
    TXT_TEXT_SPEED,
    TXT_SLOW,
    TXT_NORMAL,
    TXT_FAST,
    TXT_DISPLAY_MODE,
    TXT_WINDOWED,
    TXT_FULLSCREEN,
    TXT_BORDERLESS,
    TXT_BRIGHTNESS,
    TXT_MASTER_VOLUME,
    TXT_MUSIC,
    TXT_SOUND_EFFECTS,
    TXT_INTERFACE,
    TXT_AMBIENCE,
    TXT_DIALOGUE,
    TXT_MOVEMENT,
    TXT_UP,
    TXT_DOWN,
    TXT_LEFT,
    TXT_RIGHT,
    TXT_SKIP_LOADING,
    TXT_PAUSE_GAME,
    TXT_PRESS_A_KEY,
    TXT_EUSKARA,
    TXT_CASTELLANO,
    TXT_ENGLISH,

    TXT_COUNT
} TextId;

typedef enum {
    TEXT_BUTTON,
    TEXT_TAB,
    TEXT_HEADER,
    TEXT_LABEL,
    TEXT_VALUE,
    TEXT_MUTED,
    TEXT_CONFLICT,
    TEXT_STYLE_COUNT
} TextStyle;

void font_load(void);
void font_unload(void);
void text_set_language(Language language);
const char *text_get(TextId id);
void text_draw(const char *text, TextStyle style, Rectangle area, float scale, float offset_x, float offset_y);

#endif
