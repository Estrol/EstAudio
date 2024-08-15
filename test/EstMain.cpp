/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "EstAudio.h"
#include "EstEncoder.h"
#include <stdio.h>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

#include <windows.h>

#define _FX_TEST

int main()
{
    EST_Device *dev = EST_DeviceInit(44100, EST_DEVICE_FLAGS::EST_DEVICE_STEREO);
    if (!dev) {
        printf("Failed to init device %s\n", EST_ErrorGetMessage());
        return 1;
    }

#ifdef _NORMAL_TEST
    EST_Sample *sample = EST_SampleLoad("F:\\test.ogg");
    if (!sample) {
        printf("Failed to load sample %s\n", EST_ErrorGetMessage());
        return 1;
    }
    est_attribute_value value;
    value.attribute = EST_ATTRIB_SAMPLERATE;
    value.fValue = 44100 * 1.5f;
    value.type = EST_ATTRIB_VAL_FLOAT;

    EST_SampleSetAttribute(sample, &value);

    EST_Channel *channel = EST_SampleGetChannel(dev, sample);
    if (!channel) {
        printf("Failed to create channel %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_ChannelPlay(channel, EST_FALSE);

    while (EST_ChannelIsPlaying(channel)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EST_ChannelFree(channel);
    EST_SampleFree(sample);
#endif

#ifdef _FX_TEST
    EST_FX *fx = EST_FXLoad("F:\\test.ogg");
    if (!fx) {
        printf("Failed to load fx %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_Channel *channel = EST_FXCreateChannel(fx, dev);
    if (!channel) {
        printf("Failed to create channel %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_ChannelPlay(channel, EST_FALSE);

    est_attribute_value value = {};

    bool isPlaying = true;
    while (isPlaying) {
        char c = (char)getchar();

        switch (c) {
            case 'q':
                isPlaying = false;
                break;

            case 'w':
            {
                value.attribute = EST_ATTRIB_FX_PITCH;
                EST_ChannelGetAttribute(channel, &value);

                value.fValue = value.fValue == 1.5f ? 1.0f : 1.5f;
                value.type = EST_ATTRIB_VAL_FLOAT;
                EST_ChannelSetAttribute(channel, &value);
                break;
            }

            case 'e':
            {
                value.attribute = EST_ATTRIB_FX_TEMPO;
                EST_ChannelGetAttribute(channel, &value);
                
                value.fValue = value.fValue == 1.5f ? 1.0f : 1.5f;
                value.type = EST_ATTRIB_VAL_FLOAT;
                EST_ChannelSetAttribute(channel, &value);
                break;
            }

            case 'd':
            {
                value.attribute = EST_ATTRIB_FX_TEMPO;
                EST_ChannelGetAttribute(channel, &value);

                value.fValue = value.fValue == 0.5f ? 1.0f : 0.5f;
                value.type = EST_ATTRIB_VAL_FLOAT;
                EST_ChannelSetAttribute(channel, &value);
                break;
            }

            case 'r':
            {
                EST_ChannelSeekPosition(channel, EST_CHANNEL_POSITION_PERCENT, 0);
                break;
            }

            case 't':
            {
                EST_ChannelSeekPosition(channel, EST_CHANNEL_POSITION_PERCENT, 0.9f);
                break;
            }

            case 'y':
            {
                EST_ChannelPlay(channel, EST_TRUE);
                break;
            }
        }
    }

    EST_ChannelFree(channel);
    EST_FXFree(fx);
#endif

#ifdef _FFT_TEST
    EST_Channel *ch = EST_ChannelLoad(dev, "F:\\test.ogg");
    if (!ch) {
        printf("Failed to load channel %s\n", EST_ErrorGetMessage());
        return 1;
    }

    float fft[1024];
    int   received = EST_ChannelGetData(ch, fft, EST_FFT_1024, (EST_GET_DATA_TYPE)(EST_GET_DATA_TYPE_FFT));
    if (received < 0) {
        printf("Failed to get data %s\n", EST_ErrorGetMessage());
        return 1;
    }

    EST_ChannelFree(ch);
#endif


    EST_DeviceFree(dev);
    return 0;
}
