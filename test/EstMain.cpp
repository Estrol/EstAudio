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
    EHANDLE hhandle;
    EST_RESULT result = EST_EncoderLoad(
        "test1-origin.wav",
        NULL,//data_callback,
        EST_DECODER_STEREO,
        &hhandle);

    result = EST_EncoderSetAttribute(hhandle, EST_ATTRIB_ENCODER_TEMPO, 1.5f);

    EST_DeviceInit(44100, EST_DEVICE_STEREO);

    EHANDLE handle;
    EST_EncoderGetSample(hhandle, &handle);

    EST_SampleSetAttribute(handle, EST_ATTRIB_LOOPING, (EST_BOOL)EST_TRUE);

    EST_SamplePlay(handle);

    (void)getchar();

    EST_SampleStop(handle);
    EST_SampleFree(handle);
    EST_DeviceFree();

    return 0;
}