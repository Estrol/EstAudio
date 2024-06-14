#include "ChannelInternal.h"

EST_RESULT EST_ChannelSetAttribute(EST_Device *device, EST_Channel *handle, est_attribute_value *value)
{
    if (!device) {
        EST_ErrorSetMessage("EST_ChannelSetAttribute: device is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_ChannelSetAttribute: handle is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(handle->magic, EST_CHANNEL_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!value) {
        EST_ErrorSetMessage("EST_ChannelSetAttribute: value is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_VOLUME:
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for volume");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_gainer_set_master_volume(&handle->gainer, value->fValue);
            break;
        case EST_ATTRIB_RATE:
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for rate");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.rate = value->fValue;
            ma_resampler_set_rate(&handle->pitch->resampler, (ma_uint32)(value->fValue * (float)handle->buffer.ref.sampleRate), handle->buffer.ref.sampleRate);
            break;
        case EST_ATTRIB_PITCH:
            if (value->type != EST_ATTRIB_VAL_BOOL) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for pitch");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->pitch->isPitched = value->bValue;
            break;
        case EST_ATTRIB_PAN:
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for pan");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_panner_set_pan(&handle->panner, value->fValue);
            break;
        case EST_ATTRIB_LOOPING:
            if (value->type != EST_ATTRIB_VAL_BOOL) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for looping");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.looping = value->bValue;
            break;
        default:
            EST_ErrorSetMessage("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }
}

EST_RESULT EST_ChannelGetAttribute(EST_Device *device, EST_Channel *handle, est_attribute_value *value)
{
    if (!device) {
        EST_ErrorSetMessage("EST_ChannelGetAttribute: device is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!handle) {
        EST_ErrorSetMessage("EST_ChannelGetAttribute: handle is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!value) {
        EST_ErrorSetMessage("EST_ChannelGetAttribute: value is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_VOLUME:
            ma_result result = ma_gainer_get_master_volume(&handle->gainer, &value->fValue);
            if (result != MA_SUCCESS) {
                EST_ErrorSetMessage("Failed to get volume");
                return EST_ERROR_INVALID_OPERATION;
            }
            break;
        case EST_ATTRIB_RATE:
            value->fValue = handle->attributes.rate;
            break;
        case EST_ATTRIB_PITCH:
            value->bValue = (EST_BOOL)handle->pitch->isPitched;
            break;
        case EST_ATTRIB_PAN:
            value->fValue = ma_panner_get_pan(&handle->panner);
            break;
        case EST_ATTRIB_LOOPING:
            value->bValue = (EST_BOOL)handle->attributes.looping;
            break;
        default:
            EST_ErrorSetMessage("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }
}