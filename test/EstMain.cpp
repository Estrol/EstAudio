#include "EstAudio.h"
#include "EstEncoder.h"
#include <stdio.h>
#include <thread>
#include <chrono>

int main()
{
    EST_DEVICE_HANDLE device;
    if (EST_DeviceInit(44100, (EST_DEVICE_FLAGS)(EST_DEVICE_STEREO | EST_DEVICE_FORMAT_F32), &device) != EST_OK) {
        printf("Failed to initialize audio device %s\n", EST_GetError());
        return 1;
    }

    EST_ENCODER_HANDLE encoder;
    auto               result = EST_EncoderLoad("F:\\test.wav", nullptr, (EST_DECODER_FLAGS)0, &encoder);

    if (result != EST_OK) {
        printf("Failed to load sample %s\n", EST_GetError());
        return 1;
    }

    EST_EncoderSetAttribute(encoder, EST_ATTRIB_ENCODER_TEMPO, 1.5f);

    EST_AUDIO_HANDLE sample;
    EST_EncoderGetSample(encoder, device, &sample);

    EST_EncoderFree(encoder);

    EST_SamplePlay(device, sample);
    EST_SampleSetAttribute(device, sample, EST_ATTRIB_RATE, 1.5f);

    EST_STATUS status;
    while (EST_SampleGetStatus(device, sample, &status) == EST_OK && status != EST_STATUS_AT_END) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    EST_SampleStop(device, sample);
    EST_SampleFree(device, sample);
    EST_DeviceFree(device);

    return 0;
}