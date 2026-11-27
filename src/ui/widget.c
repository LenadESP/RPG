#include "canvas.h"
#include "settings.h"
#include "sound.h"
#include "text.h"
#include "defines.h"

#include "raylib.h"
#include <stdbool.h>
#include <stdlib.h>

//The text of each dropdown option, in the same order as the setting's values
static const TextId dropdown_options[SETTING_COUNT][DROPDOWN_OPTION_COUNT] = {
    [SETTING_TEXT_SPEED]   = {TXT_SLOW, TXT_NORMAL, TXT_FAST},
    [SETTING_DISPLAY_MODE] = {TXT_WINDOWED, TXT_FULLSCREEN, TXT_BORDERLESS},
};

//-1 when there's none
static int dragged_slider_setting = -1;
static int open_dropdown_setting = -1;
static int waiting_key_setting = -1;

static void slider_draw(const CanvasWidget *widget, Vector2 mouse_canvas, bool clicked, CanvasTransform transform){
    Rectangle area = widget->area;
    float inner_width = area.width - 2 * SLIDER_TRACK_BORDER;

    //A bit bigger than the track so it's easy to grab
    Rectangle grab_area = {area.x - 14.0f, area.y - 24.0f, area.width + 28.0f, area.height + 48.0f};

    if (clicked && CheckCollisionPointRec(mouse_canvas, grab_area)) {
        dragged_slider_setting = widget->setting;
    }

    if (dragged_slider_setting == (int)widget->setting) {
        float value = (mouse_canvas.x - area.x - SLIDER_TRACK_BORDER) / inner_width * 100.0f;

        if (value < 0.0f) value = 0.0f;
        if (value > 100.0f) value = 100.0f;

        settings_set_pending(widget->setting, (int)(value + 0.5f));
    }

    int value = settings_get_pending(widget->setting);
    float fill_width = inner_width * value / 100.0f;

    Rectangle fill_area  = {area.x + SLIDER_TRACK_BORDER, area.y + SLIDER_TRACK_BORDER, fill_width, area.height - 2 * SLIDER_TRACK_BORDER};
    Rectangle thumb_area = {fill_area.x + fill_width - SLIDER_THUMB_WIDTH / 2.0f, area.y + (area.height - SLIDER_THUMB_HEIGHT) / 2.0f, SLIDER_THUMB_WIDTH, SLIDER_THUMB_HEIGHT};
    Rectangle value_area = {area.x + area.width + 12.0f, area.y + area.height / 2.0f - 20.0f, 120.0f, 40.0f};

    canvas_draw_texture(TEX_SETTINGS_SLIDER_TRACK, area, transform);
    canvas_draw_texture(TEX_SETTINGS_SLIDER_FILL, fill_area, transform);
    canvas_draw_texture(TEX_SETTINGS_SLIDER_THUMB, thumb_area, transform);
    text_draw(TextFormat("%d%%", value), TEXT_VALUE, value_area, transform);
}

//The box is square on the left, the yes/no text goes after it
static void checkbox_draw(const CanvasWidget *widget, Vector2 mouse_canvas, bool clicked, CanvasTransform transform){
    Rectangle area = widget->area;
    bool checked = settings_get_pending(widget->setting);

    if (clicked && CheckCollisionPointRec(mouse_canvas, area)) {
        play_sound(SOUND_UI_ACCEPT, 0.4f);
        checked = !checked;
        settings_set_pending(widget->setting, checked);
    }

    Rectangle box_area  = {area.x, area.y, area.height, area.height};
    Rectangle text_area = {area.x + area.height + 14.0f, area.y, area.width - area.height - 14.0f, area.height};

    if (checked) {
        canvas_draw_texture(TEX_SETTINGS_CHECKBOX_ON, box_area, transform);
        text_draw(text_get(TXT_YES), TEXT_LABEL, text_area, transform);
    } else {
        canvas_draw_texture(TEX_SETTINGS_CHECKBOX_OFF, box_area, transform);
        text_draw(text_get(TXT_NO), TEXT_LABEL, text_area, transform);
    }
}

//Only the closed part. The open list is drawn on top of everything, in dropdown_list_draw
static void dropdown_draw(const CanvasWidget *widget, Vector2 mouse_canvas, bool clicked, CanvasTransform transform){
    Rectangle area = widget->area;
    bool hovered = CheckCollisionPointRec(mouse_canvas, area);

    if (clicked && hovered) {
        play_sound(SOUND_UI_ACCEPT, 0.4f);
        open_dropdown_setting = widget->setting;
    }

    bool open = (open_dropdown_setting == (int)widget->setting);
    int value = settings_get_pending(widget->setting);

    Rectangle text_area  = {area.x, area.y, area.width - 40.0f, area.height};
    Rectangle arrow_area = {area.x + area.width - 36.0f, area.y + area.height / 2.0f - 10.0f, 22.0f, 22.0f};

    if (open || hovered) {
        canvas_draw_texture(TEX_SETTINGS_DROPDOWN_HOVER, area, transform);
    } else {
        canvas_draw_texture(TEX_SETTINGS_DROPDOWN_NORMAL, area, transform);
    }

    if (open) {
        canvas_draw_texture(TEX_SETTINGS_ARROW_UP, arrow_area, transform);
    } else {
        canvas_draw_texture(TEX_SETTINGS_ARROW_DOWN, arrow_area, transform);
    }

    text_draw(text_get(dropdown_options[widget->setting][value]), TEXT_VALUE, text_area, transform);
}

static Rectangle dropdown_option_area(const CanvasWidget *widget, int option){
    Rectangle area = widget->area;
    return (Rectangle){area.x + 6.0f, area.y + area.height + 8.0f + 48.0f * option, area.width - 12.0f, 48.0f};
}

static void dropdown_list_draw(const CanvasWidget *widget, Vector2 mouse_canvas, CanvasTransform transform){
    Rectangle area = widget->area;
    Rectangle list_area = {area.x, area.y + area.height, area.width, 160.0f};
    int value = settings_get_pending(widget->setting);

    canvas_draw_texture(TEX_SETTINGS_DROPDOWN_LIST, list_area, transform);

    for (int option = 0; option < DROPDOWN_OPTION_COUNT; option++) {
        Rectangle option_area = dropdown_option_area(widget, option);
        Rectangle text_area = {option_area.x + 20.0f, option_area.y, option_area.width - 20.0f, option_area.height};

        if (option == value || CheckCollisionPointRec(mouse_canvas, option_area)) {
            canvas_draw_texture(TEX_SETTINGS_OPTION_HOVER, option_area, transform);
        } else {
            canvas_draw_texture(TEX_SETTINGS_OPTION_NORMAL, option_area, transform);
        }

        text_draw(text_get(dropdown_options[widget->setting][option]), TEXT_LABEL, text_area, transform);
    }
}

//The whole row is clickable, the circle sits on the left
static void radio_draw(const CanvasWidget *widget, Vector2 mouse_canvas, bool clicked, CanvasTransform transform){
    Rectangle area = widget->area;

    if (clicked && CheckCollisionPointRec(mouse_canvas, area)) {
        play_sound(SOUND_UI_ACCEPT, 0.4f);
        settings_set_pending(widget->setting, widget->option);
    }

    Rectangle circle_area = {area.x + 4.0f, area.y + area.height / 2.0f - 17.0f, 34.0f, 34.0f};

    if (settings_get_pending(widget->setting) == widget->option) {
        canvas_draw_texture(TEX_SETTINGS_RADIO_ON, circle_area, transform);
    } else {
        canvas_draw_texture(TEX_SETTINGS_RADIO_OFF, circle_area, transform);
    }
}

//Raylib has no names for its keys, so only the usual ones get a name
static const char *key_name(int key){
    if ((key >= KEY_A && key <= KEY_Z) || (key >= KEY_ZERO && key <= KEY_NINE)) {
        return TextFormat("%c", key);
    }

    if (key >= KEY_F1 && key <= KEY_F12) {
        return TextFormat("F%d", key - KEY_F1 + 1);
    }

    switch (key) {
        case KEY_SPACE:         return "Space";
        case KEY_ENTER:         return "Enter";
        case KEY_ESCAPE:        return "Esc";
        case KEY_TAB:           return "Tab";
        case KEY_BACKSPACE:     return "Back";
        case KEY_UP:            return "Up";
        case KEY_DOWN:          return "Down";
        case KEY_LEFT:          return "Left";
        case KEY_RIGHT:         return "Right";
        case KEY_LEFT_SHIFT:    return "Shift";
        case KEY_LEFT_CONTROL:  return "Ctrl";
        case KEY_LEFT_ALT:      return "Alt";
        default:                return "?";
    }
}

static void key_draw(const CanvasWidget *widget, Vector2 mouse_canvas, bool clicked, CanvasTransform transform){
    Rectangle area = widget->area;
    bool hovered = CheckCollisionPointRec(mouse_canvas, area);

    if (clicked && hovered) {
        play_sound(SOUND_UI_ACCEPT, 0.4f);
        waiting_key_setting = widget->setting;
    }

    if (waiting_key_setting == (int)widget->setting) {
        canvas_draw_texture(TEX_SETTINGS_KEYCAP_HOVER, area, transform);
        text_draw("...", TEXT_VALUE, area, transform);
        return;
    }

    if (hovered) {
        canvas_draw_texture(TEX_SETTINGS_KEYCAP_HOVER, area, transform);
    } else {
        canvas_draw_texture(TEX_SETTINGS_KEYCAP_NORMAL, area, transform);
    }

    //Red if another action has the same key
    int key = settings_get_pending(widget->setting);
    bool repeated = false;

    for (int other = SETTING_KEY_UP; other <= SETTING_KEY_PAUSE_GAME; other++) {
        if (other != (int)widget->setting && settings_get_pending(other) == key) {
            repeated = true;
        }
    }

    if (repeated) {
        text_draw(key_name(key), TEXT_CONFLICT, area, transform);
    } else {
        text_draw(key_name(key), TEXT_VALUE, area, transform);
    }
}

bool widget_waiting_for_key(void){
    return waiting_key_setting != -1;
}

void canvas_draw_widgets(const CanvasWidget *widgets, int count, Vector2 mouse_canvas, CanvasTransform transform){
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        dragged_slider_setting = -1;
    }

    //Waiting for a key: Esc or a click cancels, any other key is the new one
    if (waiting_key_setting != -1) {
        int key = GetKeyPressed();

        if (key == KEY_ESCAPE || clicked) {
            waiting_key_setting = -1;
        } else if (key != 0) {
            settings_set_pending(waiting_key_setting, key);
            waiting_key_setting = -1;
        }
    }

    //An open dropdown eats the click, so nothing under its list reacts
    const CanvasWidget *open_dropdown = NULL;

    for (int i = 0; i < count; i++) {
        if (widgets[i].type == WIDGET_DROPDOWN && (int)widgets[i].setting == open_dropdown_setting) {
            open_dropdown = &widgets[i];
        }
    }

    if (clicked && open_dropdown_setting != -1) {
        if (open_dropdown != NULL) {
            for (int option = 0; option < DROPDOWN_OPTION_COUNT; option++) {
                if (CheckCollisionPointRec(mouse_canvas, dropdown_option_area(open_dropdown, option))) {
                    play_sound(SOUND_UI_ACCEPT, 0.4f);
                    settings_set_pending(open_dropdown->setting, option);
                }
            }
        }

        open_dropdown_setting = -1;
        clicked = false;
    }

    for (int i = 0; i < count; i++) {
        switch (widgets[i].type) {
            case WIDGET_SLIDER:   slider_draw(&widgets[i], mouse_canvas, clicked, transform);   break;
            case WIDGET_CHECKBOX: checkbox_draw(&widgets[i], mouse_canvas, clicked, transform); break;
            case WIDGET_DROPDOWN: dropdown_draw(&widgets[i], mouse_canvas, clicked, transform); break;
            case WIDGET_RADIO:    radio_draw(&widgets[i], mouse_canvas, clicked, transform);    break;
            case WIDGET_KEY:      key_draw(&widgets[i], mouse_canvas, clicked, transform);      break;
        }
    }

    //Last, so the list covers whatever is under it
    if (open_dropdown != NULL && open_dropdown_setting == (int)open_dropdown->setting) {
        dropdown_list_draw(open_dropdown, mouse_canvas, transform);
    }
}
