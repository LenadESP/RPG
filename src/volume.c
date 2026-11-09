#include "volume.h"

static float volumes[CHANNEL_COUNT] = {
    [CHANNEL_MASTER] = 1.0f,
    [CHANNEL_MUSIC] = 1.0f,
    [CHANNEL_SFX] = 1.0f,
    [CHANNEL_UI] = 1.0f,
    [CHANNEL_AMBIENT] = 1.0f,
    [CHANNEL_DIALOGUE] = 1.0f,
};

float get_channel_volume(ChannelId channel) {
    return volumes[channel];
}

void set_channel_volume(ChannelId channel, float volume){
    volumes[channel] = volume;

    if(channel == CHANNEL_MASTER){
        SetMasterVolume(volume);
    }
}