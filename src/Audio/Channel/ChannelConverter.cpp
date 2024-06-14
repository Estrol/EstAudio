#include "ChannelInternal.h"
#include "../../Encoder/EncoderInternal.h"
#include "../../Utils/IO.h"

struct EST_Channel *EST_SampleGetChannel(EST_Device *device, EST_Sample *handle)
{
    if (!device) {
        EST_ErrorSetMessage("EST_SampleGetChannel: device is nullptr");
        return nullptr;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_SampleGetChannel: handle is nullptr");
        return nullptr;
    }

    if (memcmp(handle->signature, EST_SAMPLE_MAGIC, 5) == 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return nullptr;
    }

    int    channels = handle->channels;
    int    pcmSize = handle->pcmSize;
    int    sampleRate = handle->sampleRate;
    float *data = &handle->data[0];

    return InternalInit(device, data, channels, pcmSize, sampleRate);
}

int EST_SampleGetChannels(EST_Device *device, EST_Sample *handle, int howManyChannelsToCreated, EST_Channel **out)
{
    if (!device) {
        EST_ErrorSetMessage("EST_SampleGetChannels: device is nullptr");
        return 0;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_SampleGetChannels: handle is nullptr");
        return 0;
    }

    if (memcmp(handle->signature, EST_SAMPLE_MAGIC, 5) == 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return 0;
    }

    int channelsCreated = 0;

    for (int i = 0; i < howManyChannelsToCreated; i++) {
        out[i] = EST_SampleGetChannel(device, handle);

        if (out[i] == nullptr) {
            break;
        }

        channelsCreated++;
    }

    return channelsCreated;
}

struct EST_Channel *EST_EncoderGetChannel(EST_Device *device, EST_Encoder *handle)
{
    if (!device) {
        EST_ErrorSetMessage("EST_EncoderGetChannel: device is nullptr");
        return nullptr;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_SampleGetChannel: handle is nullptr");
        return nullptr;
    }

    if (memcmp(handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return nullptr;
    }

    if (handle->numOfPcmProcessed == 0) {
        EST_RESULT renderResult = EST_EncoderRender(handle);
        if (renderResult != EST_OK) {
            EST_ErrorSetMessage("EST_EncoderGetChannel: failed to render encoder");
            return nullptr;
        }
    }

    int channels = handle->channels;
    int pcmSize = handle->numOfPcmProcessed;
    int sampleRate = (int)handle->sampleRate;

    return InternalInit(device, &handle->data[0], channels, pcmSize, sampleRate);
}

int EST_EncoderGetChannels(EST_Device *device, EST_Encoder *handle, int howManyChannelsToCreated, EST_Channel **out)
{
    if (!device) {
        EST_ErrorSetMessage("EST_EncoderGetChannels: device is nullptr");
        return 0;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_EncoderGetChannels: handle is nullptr");
        return 0;
    }

    if (memcmp(handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return 0;
    }

    int channelsCreated = 0;

    for (int i = 0; i < howManyChannelsToCreated; i++) {
        out[i] = EST_EncoderGetChannel(device, handle);

        if (out[i] == nullptr) {
            break;
        }

        channelsCreated++;
    }

    return channelsCreated;
}