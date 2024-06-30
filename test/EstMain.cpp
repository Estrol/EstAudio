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

    EST_Sample *sample = EST_SampleLoad("F:\\test.wav");
    if (!sample) {
        printf("Failed to load sample %s\n", EST_ErrorGetMessage());
        return 1;
    }

    std::vector<EST_Channel *> channels(2);
    int                        size = EST_SampleGetChannels(dev, sample, 2, channels.data());
    if (size != 2) {
        printf("Failed to get channels %s\n", EST_ErrorGetMessage());
        return 1;
    }

    for (int i = 0; i < size; i++) {
        EST_ChannelPlay(channels[i], EST_TRUE);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (int i = 0; i < size; i++) {
        EST_ChannelFree(channels[i]);
    }

    EST_SampleFree(sample);
    EST_DeviceFree(dev);

    return 0;
}