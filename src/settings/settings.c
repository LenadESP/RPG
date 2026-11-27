#include "settings.h"
#include "volume.h"
#include "text.h"
#include "conf.h"
#include "defines.h"

#include "raylib.h"

//Sliders go from 0 to 100, checkboxes are 0 or 1, keys are raylib's KEY_*, the rest are their enum
static const int default_values[SETTING_COUNT] = {
    [SETTING_SHOW_HINTS]      = 1,
    [SETTING_TEXT_SPEED]      = TEXT_SPEED_NORMAL,
    [SETTING_DISPLAY_MODE]    = DISPLAY_WINDOWED,
    [SETTING_BRIGHTNESS]      = 100,
    [SETTING_VOLUME_MASTER]   = 100,
    [SETTING_VOLUME_MUSIC]    = 100,
    [SETTING_VOLUME_SFX]      = 100,
    [SETTING_VOLUME_UI]       = 100,
    [SETTING_VOLUME_AMBIENT]  = 100,
    [SETTING_VOLUME_DIALOGUE] = 100,
    [SETTING_LANGUAGE]        = LANG_EU,
    [SETTING_KEY_UP]           = KEY_W,
    [SETTING_KEY_DOWN]         = KEY_S,
    [SETTING_KEY_LEFT]         = KEY_A,
    [SETTING_KEY_RIGHT]        = KEY_D,
    [SETTING_KEY_FULLSCREEN]   = KEY_F11,
    [SETTING_KEY_SKIP_LOADING] = KEY_ENTER,
    [SETTING_KEY_PAUSE_GAME]   = KEY_ESCAPE,
};

//How each setting is called in settings.conf
static const char *const setting_names[SETTING_COUNT] = {
    [SETTING_SHOW_HINTS]       = "show_hints",
    [SETTING_TEXT_SPEED]       = "text_speed",
    [SETTING_DISPLAY_MODE]     = "display_mode",
    [SETTING_BRIGHTNESS]       = "brightness",
    [SETTING_VOLUME_MASTER]    = "volume_master",
    [SETTING_VOLUME_MUSIC]     = "volume_music",
    [SETTING_VOLUME_SFX]       = "volume_sfx",
    [SETTING_VOLUME_UI]        = "volume_ui",
    [SETTING_VOLUME_AMBIENT]   = "volume_ambient",
    [SETTING_VOLUME_DIALOGUE]  = "volume_dialogue",
    [SETTING_LANGUAGE]         = "language",
    [SETTING_KEY_UP]           = "key_up",
    [SETTING_KEY_DOWN]         = "key_down",
    [SETTING_KEY_LEFT]         = "key_left",
    [SETTING_KEY_RIGHT]        = "key_right",
    [SETTING_KEY_FULLSCREEN]   = "key_fullscreen",
    [SETTING_KEY_SKIP_LOADING] = "key_skip_loading",
    [SETTING_KEY_PAUSE_GAME]   = "key_pause_game",
};

static int applied_values[SETTING_COUNT];
static int pending_values[SETTING_COUNT];

//To go back to the same size after fullscreen
static int windowed_width = 1280;
static int windowed_height = 720;

static void apply_display_mode(int old_mode, int new_mode){
    if (old_mode == new_mode) {
        return;
    }

    //Leave the old mode first
    if (old_mode == DISPLAY_WINDOWED) {
        windowed_width = GetScreenWidth();
        windowed_height = GetScreenHeight();
    } else if (old_mode == DISPLAY_FULLSCREEN) {
        ToggleFullscreen();
    } else if (old_mode == DISPLAY_BORDERLESS) {
        ToggleBorderlessWindowed();
    }

    if (new_mode == DISPLAY_WINDOWED) {
        SetWindowSize(windowed_width, windowed_height);
    } else if (new_mode == DISPLAY_FULLSCREEN) {
        ToggleFullscreen();
    } else if (new_mode == DISPLAY_BORDERLESS) {
        ToggleBorderlessWindowed();
    }
}

static void apply_to_game(int old_display_mode){
    for (int channel = 0; channel < CHANNEL_COUNT; channel++) {
        set_channel_volume(channel, applied_values[SETTING_VOLUME_MASTER + channel] / 100.0f);
    }

    text_set_language(applied_values[SETTING_LANGUAGE]);
    apply_display_mode(old_display_mode, applied_values[SETTING_DISPLAY_MODE]);

    //Brightness is read by main every frame, and hints and text speed have nothing to use them yet
}

void settings_init(void){
    for (int i = 0; i < SETTING_COUNT; i++) {
        applied_values[i] = default_values[i];
    }

    //Whatever is in the file overwrites the defaults
    conf_read(SETTINGS_PATH, setting_names, applied_values, SETTING_COUNT);

    for (int i = 0; i < SETTING_COUNT; i++) {
        pending_values[i] = applied_values[i];
    }

    //The window always starts windowed
    apply_to_game(DISPLAY_WINDOWED);
}

int settings_get(SettingId id){
    return applied_values[id];
}

int settings_get_pending(SettingId id){
    return pending_values[id];
}

void settings_set_pending(SettingId id, int value){
    pending_values[id] = value;
}

void settings_apply(void){
    int old_display_mode = applied_values[SETTING_DISPLAY_MODE];

    for (int i = 0; i < SETTING_COUNT; i++) {
        applied_values[i] = pending_values[i];
    }

    apply_to_game(old_display_mode);
    conf_write(SETTINGS_PATH, setting_names, applied_values, SETTING_COUNT);
}

void settings_discard(void){
    for (int i = 0; i < SETTING_COUNT; i++) {
        pending_values[i] = applied_values[i];
    }
}

//Reset doesn't wait for Apply
void settings_reset(void){
    for (int i = 0; i < SETTING_COUNT; i++) {
        pending_values[i] = default_values[i];
    }

    settings_apply();
}

//F11 in game
void settings_toggle_fullscreen(void){
    int old_mode = applied_values[SETTING_DISPLAY_MODE];
    int new_mode = DISPLAY_FULLSCREEN;

    if (old_mode == DISPLAY_FULLSCREEN) {
        new_mode = DISPLAY_WINDOWED;
    }

    applied_values[SETTING_DISPLAY_MODE] = new_mode;
    pending_values[SETTING_DISPLAY_MODE] = new_mode;
    apply_display_mode(old_mode, new_mode);
    conf_write(SETTINGS_PATH, setting_names, applied_values, SETTING_COUNT);
}
