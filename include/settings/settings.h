#ifndef SETTINGS_H
#define SETTINGS_H

//The volumes go in the same order as ChannelId
typedef enum {
    SETTING_SHOW_HINTS,
    SETTING_TEXT_SPEED,
    SETTING_DISPLAY_MODE,
    SETTING_BRIGHTNESS,
    SETTING_VOLUME_MASTER,
    SETTING_VOLUME_MUSIC,
    SETTING_VOLUME_SFX,
    SETTING_VOLUME_UI,
    SETTING_VOLUME_AMBIENT,
    SETTING_VOLUME_DIALOGUE,
    SETTING_LANGUAGE,
    SETTING_KEY_UP,
    SETTING_KEY_DOWN,
    SETTING_KEY_LEFT,
    SETTING_KEY_RIGHT,
    SETTING_KEY_FULLSCREEN,
    SETTING_KEY_SKIP_LOADING,
    SETTING_KEY_PAUSE_GAME,
    SETTING_COUNT
} SettingId;

typedef enum {
    TEXT_SPEED_SLOW,
    TEXT_SPEED_NORMAL,
    TEXT_SPEED_FAST
} TextSpeed;

typedef enum {
    DISPLAY_WINDOWED,
    DISPLAY_FULLSCREEN,
    DISPLAY_BORDERLESS
} DisplayMode;

void settings_init(void);
int settings_get(SettingId id);

//The menu edits a copy. It only reaches the game on settings_apply
int settings_get_pending(SettingId id);
void settings_set_pending(SettingId id, int value);
void settings_apply(void);
void settings_discard(void);
void settings_reset(void);

void settings_toggle_fullscreen(void);

#endif
