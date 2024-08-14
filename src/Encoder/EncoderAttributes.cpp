/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "EncoderInternal.h"

EST_RESULT EST_EncoderSetAttribute(EST_Encoder *handle, est_attribute_value *value)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_ENCODER) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_ENCODER));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_PAN:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_EncoderSetAttribute: Invalid type for pan");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_panner_set_pan(&handle->panner, value->fValue);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_EncoderSetAttribute: Invalid type for volume");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            ma_gainer_set_master_volume(&handle->gainer, value->fValue);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_EncoderSetAttribute: Invalid type for tempo");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->rate = value->fValue;

            ma_uint32 originSample = handle->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(originSample * value->fValue);

            ma_resampler_set_rate(&handle->calculator, target, originSample);
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_EncoderSetAttribute: Invalid type for pitch");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->pitch = value->fValue;
            handle->processor->setTransposeFactor(value->fValue);
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_EncoderSetAttribute: Invalid type for samplerate");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            handle->sampleRate = value->fValue;

            ma_uint32 originSample = handle->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(value->fValue);

            ma_data_converter_set_rate(&handle->decoder.converter, target, originSample);
            break;
        }

        default:
        {
            EST_ErrorSetMessage("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetAttribute(EST_Encoder *handle, est_attribute_value *value)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_ENCODER) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_ENCODER));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_PAN:
        {
            value->fValue = ma_panner_get_pan(&handle->panner);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            ma_gainer_get_master_volume(&handle->gainer, &value->fValue);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            value->fValue = handle->rate;
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            value->fValue = handle->pitch;
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            value->fValue = handle->sampleRate;
            break;
        }

        default:
        {
            EST_ErrorSetMessage("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetInfo(EST_Encoder *handle, est_encoder_info *info)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(handle);
    if (unknown->type != EST_UNKNOWN_ENCODER) {
        auto msg = std::format(
            "Invalid handle: {} (Expect: {})",
            EST_UnknownTypeToString(unknown->type),
            EST_UnknownTypeToString(EST_UNKNOWN_ENCODER));

        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    info->channels = handle->channels;
    info->pcmSize = handle->numOfPcmProcessed;
    info->sampleRate = static_cast<int>(handle->sampleRate);

    return EST_OK;
}