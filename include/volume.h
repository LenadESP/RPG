#ifndef VOLUME_H
#define VOLUME_H

typedef enum {
    CHANNEL_MASTER,
    CHANNEL_SFX,
    CHANNEL_MUSIC,
    CHANNEL_UI,
    CHANNEL_WORLD,
    CHANNEL_DIALOGUE,
    CHANNEL_COUNT
} ChannelId;

float get_channel_volume(ChannelId channel);
void set_channel_volume(ChannelId channel, float volume);

#endif