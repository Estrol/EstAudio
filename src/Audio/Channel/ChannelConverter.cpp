#include "ChannelInternal.h"
#include "../../Encoder/EncoderInternal.h"
#include "../../Utils/IO.h"
#include <format>

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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_SAMPLE) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_SAMPLE));
        EST_ErrorSetMessage(msg.c_str());
        return nullptr;
    }

    int channels = handle->channels;
    int pcmSize = handle->pcmSize;
    int sampleRate = handle->sampleRate;

    EST_Channel *channel = ChannelInternalInitMemory(device, &handle->data[0], channels, pcmSize, sampleRate, handle->fx);

    channel->attributes = handle->attributes;
    ma_resampler_set_rate_ratio(&channel->resampler, channel->attributes.samplerate / channel->sampleRate);
    ma_panner_set_pan(&channel->panner, channel->attributes.pan);
    ma_gainer_set_master_volume(&channel->gainer, channel->attributes.volume);

    if (channel) {
        handle->channelsPlaying.push_back(channel);
    }

    return channel;
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_SAMPLE) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_SAMPLE));
        EST_ErrorSetMessage(msg.c_str());
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));
        EST_ErrorSetMessage(msg.c_str());
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

    handle->locked = true;
    return ChannelInternalInitMemory(device, &handle->data[0], channels, pcmSize, sampleRate, nullptr);
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));
        EST_ErrorSetMessage(msg.c_str());
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