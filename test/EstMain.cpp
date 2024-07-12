#include "EstAudio.h"
#include "EstEncoder.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

int main()
{
    EST_Device *dev = EST_DeviceInit(44100, EST_DEVICE_FLAGS::EST_DEVICE_STEREO);
    if (!dev) {
        printf("Failed to init device %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_Encoder* encoder = EST_EncoderLoad("F:\\test.ogg", nullptr, (EST_DECODER_FLAGS)0);
    if (!encoder) {
        printf("Failed to load encoder %s\n", EST_ErrorGetMessage());
        return 1;
    }

    est_attribute_value value = {
        EST_ATTRIB_ENCODER_TEMPO,
        EST_ATTRIB_VAL_FLOAT,
        1.5f
    };

    EST_EncoderSetAttribute(encoder, &value);
    EST_EncoderRender(encoder);

    EST_Sample *sample = EST_SampleLoadFromEncoder(encoder);
    if (!sample) {
        printf("Failed to load sample %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_EncoderFree(encoder);

    EST_Channel *channel = EST_SampleGetChannel(dev, sample);
    if (!channel) {
        printf("Failed to get channel %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_ChannelSeekPosition(channel, EST_CHANNEL_POSITION_TIME, 15000);

    EST_ChannelPlay(channel, EST_FALSE);

    std::this_thread::sleep_for(std::chrono::seconds(15));
    
    EST_SampleFree(sample);
    EST_DeviceFree(dev);

    return 0;
}