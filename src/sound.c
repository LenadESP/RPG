#include "sound.h"
#include "volume.h"

#include "raylib.h"

typedef struct {
    const char *path;
    ChannelId channel;
} SoundDef;

static const SoundDef sound_defs[SOUND_COUNT] = {
    [SOUND_UI_ACCEPT] = {"assets/Audio/Sounds/Menu/Accept.wav", CHANNEL_UI}
};

static Sound sounds[SOUND_COUNT];

void sound_load_all(void){
    SetMasterVolume(get_channel_volume(CHANNEL_MASTER));
    for (int i = 0; i < SOUND_COUNT; i++) {
        sounds[i] = LoadSound(sound_defs[i].path);

        if(!IsSoundValid(sounds[i])){
            TraceLog(LOG_WARNING, "ERROR! Asset %s could not load properly. Please check game integrity.", sound_defs[i].path);
        }
    }
}

void sound_unload_all(void){
    for (int i = 0; i < SOUND_COUNT; i++){
        if(IsSoundValid(sounds[i])){
            UnloadSound(sounds[i]);
        }
    }
}

void play_sound(SoundId id, float volume){
    SetSoundVolume(sounds[id], get_channel_volume(sound_defs[id].channel));
    PlaySound(sounds[id]);
}