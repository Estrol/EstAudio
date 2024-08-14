/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "ChannelInternal.h"

EST_RESULT EST_ChannelSetAttribute(EST_Channel *handle, est_attribute_value *value)
{
    if (!handle) {
        EST_ErrorSetMessage("EST_ChannelSetAttribute: handle is nullptr");
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

    if (!value) {
        EST_ErrorSetMessage("EST_ChannelSetAttribute: value is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_VOLUME:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for volume");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.volume = value->fValue;
            ma_gainer_set_master_volume(&handle->gainer, value->fValue);
            break;
        }

        case EST_ATTRIB_SAMPLERATE:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for rate");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.samplerate = value->fValue;
            ma_resampler_set_rate(&handle->resampler, (ma_uint32)handle->attributes.samplerate, handle->sampleRate);
            break;
        }

        case EST_ATTRIB_PAN:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for pan");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.pan = value->fValue;
            ma_panner_set_pan(&handle->panner, value->fValue);
            break;
        }

        case EST_ATTRIB_LOOPING:
        {
            if (value->type != EST_ATTRIB_VAL_BOOL) {
                EST_ErrorSetMessage("EST_ChannelSetAttribute: Invalid type for looping");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->attributes.looping = value->bValue;
            break;
        }

        case EST_ATTRIB_FX_PITCH:
        {
            auto fx = handle->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("Channel is not FX Channel!");
                return EST_ERROR;
            }

            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.pitch = value->fValue;
            fx->processor->setTransposeFactor(value->fValue);
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            auto fx = handle->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("Channel is not FX Channel!");
                return EST_ERROR;
            }

            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.tempo = value->fValue;

            ma_uint32 originSample = handle->sampleRate;
            ma_uint32 target = (ma_uint64)(originSample * value->fValue);

            ma_resampler_set_rate(&fx->resampler, target, originSample);
            break;
        }

        default:
            EST_ErrorSetMessage("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_ChannelGetAttribute(EST_Channel *handle, est_attribute_value *value)
{
    if (!handle) {
        EST_ErrorSetMessage("EST_ChannelGetAttribute: handle is nullptr");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!value) {
        EST_ErrorSetMessage("EST_ChannelGetAttribute: value is nullptr");
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

    switch (value->attribute) {
        case EST_ATTRIB_VOLUME:
        {
            value->fValue = handle->attributes.volume;
            break;
        }

        case EST_ATTRIB_SAMPLERATE:
        {
            value->fValue = handle->attributes.samplerate;
            break;
        }

        case EST_ATTRIB_PAN:
        {
            value->fValue = handle->attributes.pan;
            break;
        }

        case EST_ATTRIB_LOOPING:
        {
            value->bValue = (EST_BOOL)handle->attributes.looping;
            break;
        }

        case EST_ATTRIB_FX_PITCH:
        {
            auto fx = handle->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("Channel is not FX Channel!");
                return EST_ERROR;
            }

            value->fValue = fx->attributes.pitch;
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            auto fx = handle->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("Channel is not FX Channel!");
                return EST_ERROR;
            }

            value->fValue = fx->attributes.tempo;
            break;
        }

        default:
            EST_ErrorSetMessage("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}