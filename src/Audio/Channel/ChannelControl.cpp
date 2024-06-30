#include "../Internal.h"

bool IsChannelOnThisDevice(EST_Device *device, EST_Channel *channel)
{
    if (!device) {
        return false;
    }

    if (!channel) {
        return false;
    }

    EST_Unknown *unknown = (EST_Unknown *)channel;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (handle->isRemoved) {
        return EST_ERROR_INVALID_DATA;
    }

    if (handle->status == EST_STATUS_PAUSED && !restart) {
        handle->isPlaying = EST_TRUE;
        handle->status = EST_STATUS_PLAYING;
        return EST_OK;
    }

    if (handle->status == EST_STATUS_PLAYING) {
        return EST_OK;
    }

    handle->isPlaying = EST_TRUE;
    handle->status = EST_STATUS_PLAYING;

    return EST_OK;
}

EST_RESULT EST_ChannelPause(EST_Channel *handle)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
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

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_FALSE;
    }

    return (EST_BOOL)handle->isPlaying;
}

EST_RESULT EST_ChannelFree(EST_Channel *handle)
{
    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_CHANNEL) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    unknown->type = EST_UNKNOWN_NONE;
    handle->isRemoved = true;

    return EST_OK;
}