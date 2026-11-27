#include "entity.h"
#include "game.h"
#include "raylib.h"
#include "collision.h"
#include "map.h"
#include "texture.h"
#include "canvas.h"
#include "defines.h"
#include "sound.h"
#include "settings.h"

#include <stdlib.h>
#include <stdbool.h>

void game_menu_draw(void (*start_game)(void), void (*open_settings)(void), void (*close_game)(void)){
    CanvasTransform transform = canvas_get_transform();
    Vector2 mouse_canvas = canvas_mouse(transform);

    static const CanvasElement UIElements[] = {
        [0] = {TEX_MAINMENU_BG,      {  0.0f,   0.0f, 1672.0f, 941.0f}},
        [1] = {TEX_MAINMENU_LOGO,    {523.5f,  60.0f,  625.0f, 405.0f}},
        [2] = {TEX_MAINMENU_NBUTTON, {576.0f, 520.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [3] = {TEX_MAINMENU_NBUTTON, {576.0f, 650.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [4] = {TEX_MAINMENU_NBUTTON, {576.0f, 780.0f,  520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{576.0f, 520.0f, 520.0f, 100.0f}, TXT_PLAY, TEXT_BUTTON},
        [1] = {{576.0f, 650.0f, 520.0f, 100.0f}, TXT_CONFIG, TEXT_BUTTON},
        [2] = {{576.0f, 780.0f, 520.0f, 100.0f}, TXT_EXIT, TEXT_BUTTON},
    };

    int hovered_now = canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);

    // Edge detection for the hover sound. Survives leaving and re-entering the menu
    // Only on the leading edge: entering a different plaque than last frame
    static int last_hovered = -1;

    if (hovered_now != last_hovered && hovered_now != -1) {
        play_sound(SOUND_UI_HOVER, 0.4f);
    }
    last_hovered = hovered_now;

    // Plaques are UIElements 2, 3 and 4.
    // If you reorder that table, these indices lie and nothing warns you.
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_canvas, UIElements[2].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            start_game();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[3].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            open_settings();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[4].area)) {
            close_game();
        }
    }

}

void game_pause_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map,
                     void (*resume_game)(void), void (*open_settings)(void), void (*exit_to_menu)(void)){
    game_draw(entities, count, debug, camera, map);

    //Tint over the whole window, outside BeginMode2D
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    CanvasTransform transform = canvas_get_transform();
    Vector2 mouse_canvas = canvas_mouse(transform);

    //No logo here, so the plaques sit centered
    static const CanvasElement UIElements[] = {
        [0] = {TEX_MAINMENU_NBUTTON, {576.0f, 290.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [1] = {TEX_MAINMENU_NBUTTON, {576.0f, 420.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
        [2] = {TEX_MAINMENU_NBUTTON, {576.0f, 550.0f, 520.0f, 100.0f}, true, TEX_MAINMENU_HBUTTON},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{576.0f, 290.0f, 520.0f, 100.0f}, TXT_RESUME, TEXT_BUTTON},
        [1] = {{576.0f, 420.0f, 520.0f, 100.0f}, TXT_CONFIG, TEXT_BUTTON},
        [2] = {{576.0f, 550.0f, 520.0f, 100.0f}, TXT_TO_MENU, TEXT_BUTTON},
    };

    int hovered_now = canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);

    //Its own edge state: this menu's indices are not the main menu's
    static int last_hovered = -1;

    if (hovered_now != last_hovered && hovered_now != -1) {
        play_sound(SOUND_UI_HOVER, 0.4f);
    }
    last_hovered = hovered_now;

    //Click handling. Careful: indices must match the table above
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse_canvas, UIElements[0].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            resume_game();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[1].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            open_settings();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[2].area)) {
            play_sound(SOUND_UI_CANCEL, 0.4f);
            exit_to_menu();
        }
    }
}

void game_load_draw(void (*set_menu)(void)){
    static float elapsed = 0.0f;
    elapsed += GetFrameTime();

    float p = elapsed / LOADING_DURATION;
    if (p > 1.0f) p = 1.0f;

    //Smoothstep: starts and ends slowly
    float eased = p * p * (3.0f - 2.0f * p);

    //Alpha runs ahead of the sorting, so it finishes appearing first
    float alpha = eased * 1.15f;
    if (alpha > 1.0f) alpha = 1.0f;

    float amount = (1.0f - eased) * LOADING_MAX_OFFSET;

    CanvasTransform transform = canvas_get_transform();

    //Logo big and centered here. It snaps to its menu spot on the swap
    static const Rectangle bg_area   = {  0.0f,   0.0f, 1672.0f, 941.0f};
    static const Rectangle logo_area = {386.0f, 179.0f,  900.0f, 583.0f};

    draw_glitched(TEX_MAINMENU_BG,   bg_area,   alpha, amount, transform);
    draw_glitched(TEX_MAINMENU_LOGO, logo_area, alpha, amount, transform);

    if (elapsed >= LOADING_DURATION) {
        elapsed = 0.0f;
        set_menu();
    }
}

void game_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map){
    BeginMode2D(*camera);

    map_draw(map, 0, map->layer_cut);

    for (int i = 1; i < count; i++) {
        if (!entities[i].active) continue;

        entity_draw(&entities[i]);

        //Drawing the hitbox
        if(debug){
            DrawRectangleLinesEx(collision_rect(&entities[i]), 1, YELLOW);
        }
    }

    //Draw the player last
    if(entities[0].active){
        entity_draw(&entities[0]);
    }

    map_draw(map, map->layer_cut, map->layer_amount);

    EndMode2D();
}
typedef enum {
    TAB_GENERAL,
    TAB_VIDEO,
    TAB_AUDIO,
    TAB_CONTROLS,
    TAB_LANGUAGE,
    TAB_COUNT
} SettingsTab;

//One function per tab, so the editor can open each one as its own screen
static void settings_general_draw(Vector2 mouse_canvas, CanvasTransform transform){
    static const CanvasElement UIElements[] = {
        [0] = {TEX_SETTINGS_SEPARATOR, {564.0f, 422.0f, 868.0f, 10.0f}},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{564.0f, 246.0f, 868.0f, 40.0f}, TXT_GENERAL,    TEXT_HEADER},
        [1] = {{564.0f, 342.0f, 400.0f, 40.0f}, TXT_SHOW_HINTS, TEXT_LABEL},
        [2] = {{564.0f, 470.0f, 400.0f, 40.0f}, TXT_TEXT_SPEED, TEXT_LABEL},
    };

    static const CanvasWidget UIWidgets[] = {
        [0] = {WIDGET_CHECKBOX, {1254.0f, 342.0f, 178.0f, 40.0f}, SETTING_SHOW_HINTS},
        [1] = {WIDGET_DROPDOWN, {1048.0f, 464.0f, 384.0f, 54.0f}, SETTING_TEXT_SPEED},
    };

    canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);
    canvas_draw_widgets(UIWidgets, sizeof(UIWidgets) / sizeof(UIWidgets[0]), mouse_canvas, transform);
}

static void settings_video_draw(Vector2 mouse_canvas, CanvasTransform transform){
    static const CanvasElement UIElements[] = {
        [0] = {TEX_SETTINGS_SEPARATOR, {564.0f, 422.0f, 868.0f, 10.0f}},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{564.0f, 246.0f, 868.0f, 40.0f}, TXT_VIDEO,        TEXT_HEADER},
        [1] = {{564.0f, 342.0f, 400.0f, 40.0f}, TXT_DISPLAY_MODE, TEXT_LABEL},
        [2] = {{564.0f, 470.0f, 300.0f, 40.0f}, TXT_BRIGHTNESS,   TEXT_LABEL},
    };

    static const CanvasWidget UIWidgets[] = {
        [0] = {WIDGET_SLIDER,   { 892.0f, 482.0f, 408.0f, 18.0f}, SETTING_BRIGHTNESS},
        [1] = {WIDGET_DROPDOWN, {1048.0f, 336.0f, 384.0f, 54.0f}, SETTING_DISPLAY_MODE},
    };

    canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);
    canvas_draw_widgets(UIWidgets, sizeof(UIWidgets) / sizeof(UIWidgets[0]), mouse_canvas, transform);
}

static void settings_audio_draw(Vector2 mouse_canvas, CanvasTransform transform){
    static const CanvasElement UIElements[] = {
        [0] = {TEX_SETTINGS_SEPARATOR, {564.0f, 374.0f, 868.0f, 10.0f}},
        [1] = {TEX_SETTINGS_SEPARATOR, {564.0f, 442.0f, 868.0f, 10.0f}},
        [2] = {TEX_SETTINGS_SEPARATOR, {564.0f, 510.0f, 868.0f, 10.0f}},
        [3] = {TEX_SETTINGS_SEPARATOR, {564.0f, 578.0f, 868.0f, 10.0f}},
        [4] = {TEX_SETTINGS_SEPARATOR, {564.0f, 646.0f, 868.0f, 10.0f}},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{564.0f, 246.0f, 868.0f, 40.0f}, TXT_AUDIO,         TEXT_HEADER},
        [1] = {{564.0f, 322.0f, 300.0f, 40.0f}, TXT_MASTER_VOLUME, TEXT_LABEL},
        [2] = {{564.0f, 390.0f, 300.0f, 40.0f}, TXT_MUSIC,         TEXT_LABEL},
        [3] = {{564.0f, 458.0f, 300.0f, 40.0f}, TXT_SOUND_EFFECTS, TEXT_LABEL},
        [4] = {{564.0f, 526.0f, 300.0f, 40.0f}, TXT_INTERFACE,     TEXT_LABEL},
        [5] = {{564.0f, 594.0f, 300.0f, 40.0f}, TXT_AMBIENCE,      TEXT_LABEL},
        [6] = {{564.0f, 662.0f, 300.0f, 40.0f}, TXT_DIALOGUE,      TEXT_LABEL},
    };

    static const CanvasWidget UIWidgets[] = {
        [0] = {WIDGET_SLIDER, {892.0f, 334.0f, 408.0f, 18.0f}, SETTING_VOLUME_MASTER},
        [1] = {WIDGET_SLIDER, {892.0f, 402.0f, 408.0f, 18.0f}, SETTING_VOLUME_MUSIC},
        [2] = {WIDGET_SLIDER, {892.0f, 470.0f, 408.0f, 18.0f}, SETTING_VOLUME_SFX},
        [3] = {WIDGET_SLIDER, {892.0f, 538.0f, 408.0f, 18.0f}, SETTING_VOLUME_UI},
        [4] = {WIDGET_SLIDER, {892.0f, 606.0f, 408.0f, 18.0f}, SETTING_VOLUME_AMBIENT},
        [5] = {WIDGET_SLIDER, {892.0f, 674.0f, 408.0f, 18.0f}, SETTING_VOLUME_DIALOGUE},
    };

    canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);
    canvas_draw_widgets(UIWidgets, sizeof(UIWidgets) / sizeof(UIWidgets[0]), mouse_canvas, transform);
}

static void settings_controls_draw(Vector2 mouse_canvas, CanvasTransform transform){
    static const CanvasElement UIElements[] = {
        [0] = {TEX_SETTINGS_SEPARATOR, { 564.0f, 358.0f, 410.0f, 10.0f}},
        [1] = {TEX_SETTINGS_SEPARATOR, {1014.0f, 358.0f, 418.0f, 10.0f}},
        [2] = {TEX_SETTINGS_SEPARATOR, { 564.0f, 432.0f, 410.0f, 10.0f}},
        [3] = {TEX_SETTINGS_SEPARATOR, { 564.0f, 500.0f, 410.0f, 10.0f}},
        [4] = {TEX_SETTINGS_SEPARATOR, { 564.0f, 568.0f, 410.0f, 10.0f}},
        [5] = {TEX_SETTINGS_SEPARATOR, { 564.0f, 636.0f, 410.0f, 10.0f}},
        [6] = {TEX_SETTINGS_SEPARATOR, {1014.0f, 432.0f, 418.0f, 10.0f}},
        [7] = {TEX_SETTINGS_SEPARATOR, {1014.0f, 500.0f, 418.0f, 10.0f}},
        [8] = {TEX_SETTINGS_SEPARATOR, {1014.0f, 568.0f, 418.0f, 10.0f}},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{ 564.0f, 246.0f, 868.0f, 40.0f}, TXT_CONTROLS,     TEXT_HEADER},
        [1] = {{ 564.0f, 316.0f, 400.0f, 40.0f}, TXT_MOVEMENT,     TEXT_MUTED},
        [2] = {{1014.0f, 316.0f, 400.0f, 40.0f}, TXT_INTERFACE,    TEXT_MUTED},
        [3] = {{ 564.0f, 380.0f, 260.0f, 40.0f}, TXT_UP,           TEXT_LABEL},
        [4] = {{ 564.0f, 448.0f, 260.0f, 40.0f}, TXT_DOWN,         TEXT_LABEL},
        [5] = {{ 564.0f, 516.0f, 260.0f, 40.0f}, TXT_LEFT,         TEXT_LABEL},
        [6] = {{ 564.0f, 584.0f, 260.0f, 40.0f}, TXT_RIGHT,        TEXT_LABEL},
        [7] = {{1014.0f, 380.0f, 260.0f, 40.0f}, TXT_FULLSCREEN,   TEXT_LABEL},
        [8] = {{1014.0f, 448.0f, 260.0f, 40.0f}, TXT_SKIP_LOADING, TEXT_LABEL},
        [9] = {{1014.0f, 516.0f, 260.0f, 40.0f}, TXT_PAUSE_GAME,   TEXT_LABEL},
    };

    static const CanvasWidget UIWidgets[] = {
        [0] = {WIDGET_KEY, { 844.0f, 376.0f, 130.0f, 48.0f}, SETTING_KEY_UP},
        [1] = {WIDGET_KEY, { 844.0f, 444.0f, 130.0f, 48.0f}, SETTING_KEY_DOWN},
        [2] = {WIDGET_KEY, { 844.0f, 512.0f, 130.0f, 48.0f}, SETTING_KEY_LEFT},
        [3] = {WIDGET_KEY, { 844.0f, 580.0f, 130.0f, 48.0f}, SETTING_KEY_RIGHT},
        [4] = {WIDGET_KEY, {1294.0f, 376.0f, 130.0f, 48.0f}, SETTING_KEY_FULLSCREEN},
        [5] = {WIDGET_KEY, {1294.0f, 444.0f, 130.0f, 48.0f}, SETTING_KEY_SKIP_LOADING},
        [6] = {WIDGET_KEY, {1294.0f, 512.0f, 130.0f, 48.0f}, SETTING_KEY_PAUSE_GAME},
    };

    canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);
    canvas_draw_widgets(UIWidgets, sizeof(UIWidgets) / sizeof(UIWidgets[0]), mouse_canvas, transform);

    if (widget_waiting_for_key()) {
        static const Rectangle hint_area = {564.0f, 688.0f, 868.0f, 40.0f};
        text_draw(text_get(TXT_PRESS_A_KEY), TEXT_VALUE, hint_area, transform);
    }
}

static void settings_language_draw(Vector2 mouse_canvas, CanvasTransform transform){
    static const CanvasElement UIElements[] = {
        [0] = {TEX_SETTINGS_SEPARATOR, {564.0f, 388.0f, 868.0f, 10.0f}},
        [1] = {TEX_SETTINGS_SEPARATOR, {564.0f, 482.0f, 868.0f, 10.0f}},
        [2] = {TEX_SETTINGS_SEPARATOR, {564.0f, 576.0f, 868.0f, 10.0f}},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{564.0f, 246.0f, 868.0f, 40.0f}, TXT_LANGUAGE,   TEXT_HEADER},
        [1] = {{624.0f, 328.0f, 400.0f, 40.0f}, TXT_EUSKARA,    TEXT_HEADER},
        [2] = {{624.0f, 422.0f, 400.0f, 40.0f}, TXT_CASTELLANO, TEXT_HEADER},
        [3] = {{624.0f, 516.0f, 400.0f, 40.0f}, TXT_ENGLISH,    TEXT_HEADER},
    };

    static const CanvasWidget UIWidgets[] = {
        [0] = {WIDGET_RADIO, {564.0f, 316.0f, 868.0f, 64.0f}, SETTING_LANGUAGE, LANG_EU},
        [1] = {WIDGET_RADIO, {564.0f, 410.0f, 868.0f, 64.0f}, SETTING_LANGUAGE, LANG_ES},
        [2] = {WIDGET_RADIO, {564.0f, 504.0f, 868.0f, 64.0f}, SETTING_LANGUAGE, LANG_EN},
    };

    canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);
    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);
    canvas_draw_widgets(UIWidgets, sizeof(UIWidgets) / sizeof(UIWidgets[0]), mouse_canvas, transform);
}

void game_settings_draw(const Entity *entities, int count, bool debug, Camera2D *camera, const Map *map,
                        bool opened_from_pause, void (*close_settings)(void)){
    static SettingsTab current_tab = TAB_GENERAL;

    CanvasTransform transform = canvas_get_transform();
    Vector2 mouse_canvas = canvas_mouse(transform);

    //Behind the panel: the paused game, or the main menu background
    if (opened_from_pause) {
        game_draw(entities, count, debug, camera, map);
    } else {
        static const Rectangle bg_area = {0.0f, 0.0f, 1672.0f, 941.0f};
        canvas_draw_texture(TEX_MAINMENU_BG, bg_area, transform);
    }

    //Tint over the whole window, same as the pause
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));

    static const CanvasElement UIElements[] = {
        [0]  = {TEX_MAINMENU_LOGO,               { 724.0f,   1.0f,  224.0f, 145.0f}},
        [1]  = {TEX_SETTINGS_PANEL,              { 150.0f, 182.0f, 1372.0f, 684.0f}},
        [2]  = {TEX_SETTINGS_TITLE,              { 456.0f, 138.0f,  760.0f,  96.0f}},
        [3]  = {TEX_SETTINGS_SEPARATOR_VERTICAL, { 516.0f, 248.0f,   10.0f, 448.0f}},
        [4]  = {TEX_SETTINGS_SEPARATOR,          { 564.0f, 296.0f,  868.0f,  10.0f}},
        [5]  = {TEX_SETTINGS_SEPARATOR,          { 188.0f, 768.0f, 1294.0f,  10.0f}},
        [6]  = {TEX_SETTINGS_TAB_NORMAL,         { 188.0f, 252.0f,  304.0f,  64.0f}, true, TEX_SETTINGS_TAB_HOVER},
        [7]  = {TEX_SETTINGS_TAB_NORMAL,         { 188.0f, 338.0f,  304.0f,  64.0f}, true, TEX_SETTINGS_TAB_HOVER},
        [8]  = {TEX_SETTINGS_TAB_NORMAL,         { 188.0f, 424.0f,  304.0f,  64.0f}, true, TEX_SETTINGS_TAB_HOVER},
        [9]  = {TEX_SETTINGS_TAB_NORMAL,         { 188.0f, 510.0f,  304.0f,  64.0f}, true, TEX_SETTINGS_TAB_HOVER},
        [10] = {TEX_SETTINGS_TAB_NORMAL,         { 188.0f, 596.0f,  304.0f,  64.0f}, true, TEX_SETTINGS_TAB_HOVER},
        [11] = {TEX_SETTINGS_ACTION_NORMAL,      { 208.0f, 786.0f,  388.0f,  70.0f}, true, TEX_SETTINGS_ACTION_HOVER},
        [12] = {TEX_SETTINGS_ACTION_NORMAL,      { 642.0f, 786.0f,  388.0f,  70.0f}, true, TEX_SETTINGS_ACTION_HOVER},
        [13] = {TEX_SETTINGS_ACTION_NORMAL,      {1076.0f, 786.0f,  388.0f,  70.0f}, true, TEX_SETTINGS_ACTION_HOVER},
    };

    static const CanvasElementText UITexts[] = {
        [0] = {{ 558.0f, 144.0f, 556.0f, 84.0f}, TXT_CONFIG,   TEXT_BUTTON},
        [1] = {{ 188.0f, 252.0f, 304.0f, 64.0f}, TXT_GENERAL,  TEXT_TAB},
        [2] = {{ 188.0f, 338.0f, 304.0f, 64.0f}, TXT_VIDEO,    TEXT_TAB},
        [3] = {{ 188.0f, 424.0f, 304.0f, 64.0f}, TXT_AUDIO,    TEXT_TAB},
        [4] = {{ 188.0f, 510.0f, 304.0f, 64.0f}, TXT_CONTROLS, TEXT_TAB},
        [5] = {{ 188.0f, 596.0f, 304.0f, 64.0f}, TXT_LANGUAGE, TEXT_TAB},
        [6] = {{ 208.0f, 786.0f, 388.0f, 70.0f}, TXT_APPLY,    TEXT_TAB},
        [7] = {{ 642.0f, 786.0f, 388.0f, 70.0f}, TXT_CANCEL,   TEXT_TAB},
        [8] = {{1076.0f, 786.0f, 388.0f, 70.0f}, TXT_RESET,    TEXT_TAB},
    };

    //Tabs are UIElements 6 to 10, in the same order as SettingsTab. Then apply, cancel and reset.
    //If you reorder that table, these lie and nothing warns you.
    const int first_tab = 6;
    const int apply_button = 11;
    const int cancel_button = 12;
    const int reset_button = 13;

    int hovered_now = canvas_draw_elements(UIElements, sizeof(UIElements) / sizeof(UIElements[0]), mouse_canvas, transform);

    //The open tab always looks hovered
    canvas_draw_texture(TEX_SETTINGS_TAB_HOVER, UIElements[first_tab + current_tab].area, transform);

    canvas_draw_texts(UITexts, sizeof(UITexts) / sizeof(UITexts[0]), transform);

    switch (current_tab) {
        case TAB_GENERAL:  settings_general_draw(mouse_canvas, transform);  break;
        case TAB_VIDEO:    settings_video_draw(mouse_canvas, transform);    break;
        case TAB_AUDIO:    settings_audio_draw(mouse_canvas, transform);    break;
        case TAB_CONTROLS: settings_controls_draw(mouse_canvas, transform); break;
        case TAB_LANGUAGE: settings_language_draw(mouse_canvas, transform); break;
        case TAB_COUNT:    break;
    }

    //Its own edge state, like the other menus
    static int last_hovered = -1;

    if (hovered_now != last_hovered && hovered_now != -1) {
        play_sound(SOUND_UI_HOVER, 0.4f);
    }
    last_hovered = hovered_now;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int tab = 0; tab < TAB_COUNT; tab++) {
            if (CheckCollisionPointRec(mouse_canvas, UIElements[first_tab + tab].area)) {
                play_sound(SOUND_UI_ACCEPT, 0.4f);
                current_tab = tab;
            }
        }

        if (CheckCollisionPointRec(mouse_canvas, UIElements[apply_button].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            settings_apply();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[cancel_button].area)) {
            play_sound(SOUND_UI_CANCEL, 0.4f);
            close_settings();
        } else if (CheckCollisionPointRec(mouse_canvas, UIElements[reset_button].area)) {
            play_sound(SOUND_UI_ACCEPT, 0.4f);
            settings_reset();
        }
    }
}
