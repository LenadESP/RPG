#ifndef SOUND_H
#define SOUND_H

typedef enum {
    SOUND_UI_ACCEPT,
    SOUND_UI_CANCEL,
    SOUND_UI_HOVER,
    SOUND_COUNT
} SoundId;

void sound_load_all(void);
void sound_unload_all(void);
void play_sound(SoundId id, float volume);

#endif