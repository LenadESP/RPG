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
    TXT_COUNT
} TextId;

void font_load(void);
void font_unload(void);
void text_set_language(Language language);
const char *text_get(TextId id);
void text_draw(TextId id, Rectangle area, float scale, float offset_x, float offset_y);

#endif
