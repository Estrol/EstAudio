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
    ECHANDLE   hhandle;
    EST_RESULT result = EST_EncoderLoad(
        "C:\\Users\\ACER\\Documents\\Games\\DPJAM\\Music\\dump_output\\Back_to_the_gatemp3_ref(0).wav",
        NULL, // data_callback,
        EST_DECODER_STEREO,
        &hhandle);

    result = EST_EncoderSetAttribute(hhandle, EST_ATTRIB_ENCODER_TEMPO, 1.5f);

    EST_DeviceInit(44100, EST_DEVICE_STEREO);

    EHANDLE handle;
    EST_EncoderGetSample(hhandle, &handle);
    EST_EncoderFree(hhandle);

    EST_SampleSetAttribute(handle, EST_ATTRIB_LOOPING, (EST_BOOL)EST_TRUE);

    EST_SamplePlay(handle);

    while (true) {
        char t = (char)getchar();

        if (t == 'q') {
            EST_SampleSlideAttributeAsync(handle, EST_ATTRIB_RATE, 1.5f, 2);
        }

        if (t == 'w') {
            EST_SampleSlideAttributeAsync(handle, EST_ATTRIB_RATE, 1.0f, 2);
        }

        if (t == 'e') {
            break;
        }
    }

    EST_SampleStop(handle);
    EST_SampleFree(handle);
    EST_DeviceFree();

    return 0;
}