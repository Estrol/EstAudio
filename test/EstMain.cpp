#include "EstAudio.h"
#include "EstEncoder.h"
#include <stdio.h>

void data_callback(EHANDLE pHandle, void *pUserData, void *pData, int frameCount)
{
    float *data = reinterpret_cast<float *>(pData);

    for (int i = 0; i < frameCount * 2; i++) {
        data[i] *= 0.25f;
    }

    (void)pUserData;
    (void)pHandle;
}

int main()
{
    auto result = EST_DeviceInit(44100, EST_DEVICE_STEREO);
    if (result != EST_OK) {
        printf("Failed to initialize device\n");
        return 1;
    }

    EHANDLE handle;
    result = EST_EncoderLoad(
        "test.wav",
        data_callback,
        EST_DECODER_STEREO,
        &handle);

    result = EST_EncoderSetAttribute(handle, EST_ATTRIB_ENCODER_TEMPO, 1.5f);

    EHANDLE outHandle;
    EST_EncoderGetSample(handle, &outHandle);

    EST_EncoderFree(handle);

    EST_SamplePlay(outHandle);

    printf("Press Enter to quit...");
    (void)getchar();

    EST_SampleFree(outHandle);

    EST_DeviceFree();

    return 0;
}