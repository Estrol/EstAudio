#include "EstAudio.h"
#include "EstEncoder.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include <vector>

int main()
{
    EST_Device *dev = EST_DeviceInit(44100, EST_DEVICE_FLAGS::EST_DEVICE_STEREO);
    if (!dev) {
        printf("Failed to init device %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_Encoder *encoder = EST_EncoderLoad("F:\\test.wav", nullptr, EST_DECODER_FLAGS::EST_DECODER_UNKNOWN);
    if (!encoder) {
        printf("Failed to load encoder %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_EncoderSetAttribute(encoder, EST_ATTRIB_ENCODER_TEMPO, 1.5f);

    EST_EncoderRender(encoder);

    EST_Channel *channel = EST_EncoderGetChannel(dev, encoder);
    if (!channel) {
        printf("Failed to get channel %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_ChannelPlay(dev, channel, EST_FALSE);

    while (EST_ChannelIsPlaying(dev, channel)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EST_ChannelStop(dev, channel);
    EST_ChannelFree(dev, channel);

    EST_EncoderFree(encoder);
    EST_DeviceFree(dev);

    return 0;
}