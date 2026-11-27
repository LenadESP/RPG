#ifndef CANVAS_H
#define CANVAS_H

#include "texture.h"
#include "text.h"
#include "settings.h"
#include "raylib.h"

#include <stdbool.h>

typedef struct CanvasElement {
    TextureId normal_texture;
    Rectangle area;
    bool hover_effect;
    TextureId hover_texture;
} CanvasElement;

typedef struct CanvasElementText{
    Rectangle area;
    TextId text;
    TextStyle style;
} CanvasElementText;

typedef enum {
    WIDGET_SLIDER,
    WIDGET_CHECKBOX,
    WIDGET_DROPDOWN,
    WIDGET_RADIO,
    WIDGET_KEY
} WidgetType;

//option is only for radios: the value the setting takes when you pick it
typedef struct CanvasWidget {
    WidgetType type;
    Rectangle area;
    SettingId setting;
    int option;
} CanvasWidget;

Rectangle canvas_to_screen(Rectangle area, float scale, float offset_x, float offset_y);
void canvas_draw_texture(TextureId id, Rectangle area, float scale, float offset_x, float offset_y);
int canvas_draw_elements(const CanvasElement *elements, int count, Vector2 mouse_canvas, float scale, float offset_x, float offset_y);
void canvas_draw_texts(const CanvasElementText *texts, int count, float scale, float offset_x, float offset_y);
void canvas_draw_widgets(const CanvasWidget *widgets, int count, Vector2 mouse_canvas, float scale, float offset_x, float offset_y);
bool widget_waiting_for_key(void);
void draw_glitched (TextureId id, Rectangle area, float alpha, float amount, float scale, float offset_x, float offset_y);


#endif
