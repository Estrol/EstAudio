#include "EncoderInternal.h"

EST_RESULT EST_EncoderSetAttribute(EST_Encoder *handle, enum EST_ATTRIBUTE_FLAGS attribute, float value)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_PAN:
        {
            ma_panner_set_pan(&handle->panner, value);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            ma_gainer_set_master_volume(&handle->gainer, value);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            handle->rate = value;

            ma_uint32 originSample = handle->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(originSample * value);

            ma_resampler_set_rate(&handle->calculator, target, originSample);
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            handle->pitch = value;
            handle->processor->setTransposeFactor(value);
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            handle->sampleRate = value;

            ma_uint32 originSample = handle->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(value);

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

EST_RESULT EST_EncoderGetAttribute(EST_Encoder *handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_PAN:
        {
            *value = ma_panner_get_pan(&handle->panner);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            ma_gainer_get_master_volume(&handle->gainer, value);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            *value = handle->rate;
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            *value = handle->pitch;
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            *value = handle->sampleRate;
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

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    info->channels = handle->channels;
    info->pcmSize = handle->numOfPcmProcessed;
    info->sampleRate = static_cast<int>(handle->sampleRate);

    return EST_OK;
}