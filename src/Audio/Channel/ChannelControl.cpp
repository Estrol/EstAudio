/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "../Internal.h"

bool IsChannelOnThisDevice(EST_Device *device, EST_Channel *channel)
{
    if (!device) {
        return false;
    }

    if (!channel) {
        return false;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(channel);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));

        EST_ErrorSetMessage(msg.c_str());
        return false;
    }

    for (int i = 0; i < device->channel_arrays.size(); i++) {
        if (device->channel_arrays[i].get() == channel) {
            return true;
        }
    }

    return true;
}

EST_RESULT EST_ChannelPlay(EST_Channel *handle, EST_BOOL restart)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (handle->isRemoved) {
        return EST_ERROR_INVALID_DATA;
    }

    if (handle->status == EST_STATUS_PAUSED && handle->isPlaying && !restart) {
        handle->isPlaying = EST_TRUE;
        handle->status = EST_STATUS_PLAYING;
        return EST_OK;
    }

    if (handle->status == EST_STATUS_PLAYING && handle->isPlaying) {
        return EST_OK;
    }

    EST_ChannelSeekPosition(handle, EST_CHANNEL_POSITION_SAMPLES, 0);

    handle->isPlaying = EST_TRUE;
    handle->status = EST_STATUS_PLAYING;

    return EST_OK;
}

EST_RESULT EST_ChannelPause(EST_Channel *handle)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    handle->isPlaying = EST_FALSE;
    handle->status = EST_STATUS_PAUSED;

    return EST_OK;
}

EST_RESULT EST_ChannelStop(EST_Channel *handle)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    handle->isPlaying = EST_FALSE;
    handle->status = EST_STATUS_IDLE;

    return EST_OK;
}

EST_BOOL EST_ChannelIsPlaying(EST_Channel *handle)
{
    if (!handle) {
        return EST_FALSE;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));

        EST_ErrorSetMessage(msg.c_str());
        return EST_FALSE;
    }

    return (EST_BOOL)handle->isPlaying;
}

EST_RESULT EST_ChannelFree(EST_Channel *handle)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_CHANNEL));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    // Stop the channel first
    EST_ChannelStop(handle);

    unknown->type = EST_UNKNOWN_NONE;
    handle->isRemoved = true;

    return EST_OK;
}

EST_DataCallback *EST_ChannelAddCallback(EST_Channel *handle, EST_DATA_CALLBACK callback, void *userData)
{
    if (!handle) {
        return nullptr;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return nullptr;
    }

    EST_DataCallback callbackData;
    callbackData.callback = callback;
    callbackData.userdata = userData;

    handle->callbacks.push_back(callbackData);

    return &handle->callbacks[handle->callbacks.size() - 1];
}

EST_RESULT EST_ChannelRemoveCallback(EST_Channel *handle, EST_DataCallback *callback)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    unknown = reinterpret_cast<EST_Unknown *>(callback);
    if (unknown->type != EST_UNKNOWN_DATA_CALLBACK) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    for (int i = 0; i < handle->callbacks.size(); i++) {
        if (&handle->callbacks[i] == callback) {
            handle->callbacks.erase(handle->callbacks.begin() + i);
            return EST_OK;
        }
    }

    return EST_ERROR_INVALID_ARGUMENT;
}

static void SeekFX(EST_Channel *handle)
{
    auto fx = handle->fx;
    fx->lock = true;
    fx->processor->reset();

    // Get required sample size for initial buffer
    int inputLatency = fx->processor->inputLatency();
    int framesRequiredForInputBuffer = inputLatency + static_cast<int>(inputLatency / fx->attributes.tempo);

    std::vector<float> buffer(framesRequiredForInputBuffer * handle->channels, 0.0f);

    // Read and seek the buffer
    ma_uint64 framesRead = ma_audio_buffer_read_pcm_frames(
        &handle->buffer,
        &buffer[0],
        framesRequiredForInputBuffer,
        MA_FALSE);

    framesRequiredForInputBuffer = static_cast<int>(framesRead);

    if (framesRequiredForInputBuffer == 0) {
        return;
    }

    // Initialize the dummy buffer
    std::vector<float> temp(framesRequiredForInputBuffer * handle->channels, 0.0f);
    fx->processor->process(
        buffer,
        framesRequiredForInputBuffer,
        temp,
        framesRequiredForInputBuffer);

    fx->lock = false;
}

EST_RESULT EST_ChannelSeekPosition(EST_Channel *handle, EST_CHANNEL_POSITION_TYPE type, float position)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (type) {
        case EST_CHANNEL_POSITION_PERCENT:
        {
            if (position < 0.0f || position > 1.0f) {
                EST_ErrorSetMessage("Invalid position");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_uint64 maxFrame;
            ma_result result = ma_audio_buffer_get_available_frames(&handle->buffer, &maxFrame);
            if (result != MA_SUCCESS) {
                EST_ErrorSetMessage("Failed to get available frames");
                return EST_ERROR;
            }

            ma_uint64 framePos = static_cast<ma_uint64>(position * maxFrame);
            ma_audio_buffer_seek_to_pcm_frame(&handle->buffer, framePos);

            if (handle->fx != nullptr) {
                SeekFX(handle);
            }

            break;
        }

        case EST_CHANNEL_POSITION_SAMPLES:
        {
            if (position < 0.0f) {
                EST_ErrorSetMessage("Invalid position");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_audio_buffer_seek_to_pcm_frame(&handle->buffer, static_cast<ma_uint64>(position));

            if (handle->fx != nullptr) {
                SeekFX(handle);
            }

            break;
        }

        case EST_CHANNEL_POSITION_TIME:
        {
            if (position < 0.0f) {
                EST_ErrorSetMessage("Invalid position");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_uint64 framePos = static_cast<ma_uint64>(position * (static_cast<float>(handle->sampleRate) / 1000.0f));
            ma_audio_buffer_seek_to_pcm_frame(&handle->buffer, framePos);

            if (handle->fx != nullptr) {
                SeekFX(handle);
            }

            break;
        }

        default:
            EST_ErrorSetMessage("Invalid position type");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}