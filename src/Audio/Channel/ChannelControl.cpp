#include "../Internal.h"

bool IsChannelOnThisDevice(EST_Device *device, EST_Channel *channel)
{
    if (!device) {
        return false;
    }

    if (!channel) {
        return false;
    }

    if (memcmp(channel->magic, EST_CHANNEL_MAGIC, 5) != 0) {
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

EST_RESULT EST_ChannelPlay(EST_Device *device, EST_Channel *handle, EST_BOOL restart)
{
    if (!device) {
        return EST_ERROR_INVALID_STATE;
    }

    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!IsChannelOnThisDevice(device, handle)) {
        return EST_ERROR_INVALID_DATA;
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

EST_RESULT EST_ChannelPause(EST_Device *device, EST_Channel *handle)
{
    if (!device) {
        return EST_ERROR_INVALID_STATE;
    }

    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!IsChannelOnThisDevice(device, handle)) {
        return EST_ERROR_INVALID_DATA;
    }

    handle->isPlaying = EST_FALSE;
    handle->status = EST_STATUS_PAUSED;

    return EST_OK;
}

EST_RESULT EST_ChannelStop(EST_Device *device, EST_Channel *handle)
{
    if (!device) {
        return EST_ERROR_INVALID_STATE;
    }

    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!IsChannelOnThisDevice(device, handle)) {
        return EST_ERROR_INVALID_DATA;
    }

    handle->isPlaying = EST_FALSE;
    handle->status = EST_STATUS_IDLE;

    return EST_OK;
}

EST_BOOL EST_ChannelIsPlaying(EST_Device *device, EST_Channel *handle)
{
    if (!device) {
        return EST_FALSE;
    }

    if (!handle) {
        return EST_FALSE;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_FALSE;
    }

    if (!IsChannelOnThisDevice(device, handle)) {
        return EST_FALSE;
    }

    return (EST_BOOL)handle->isPlaying;
}

EST_RESULT EST_ChannelFree(EST_Device *device, EST_Channel *handle)
{
    if (!device) {
        return EST_ERROR_INVALID_STATE;
    }

    if (!handle) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!IsChannelOnThisDevice(device, handle)) {
        return EST_ERROR_INVALID_DATA;
    }

    memset((void *)handle->magic, 0, 5);
    handle->isRemoved = true;

    return EST_OK;
}