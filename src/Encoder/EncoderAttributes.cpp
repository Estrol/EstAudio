#include "EncoderInternal.h"

EST_RESULT EST_EncoderSetAttribute(EST_ENCODER_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_PAN:
        {
            ma_panner_set_pan(&decoder->panner, value);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            ma_gainer_set_master_volume(&decoder->gainer, value);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            decoder->rate = value;

            ma_uint32 originSample = decoder->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(originSample * value);

            ma_resampler_set_rate(&decoder->calculator, target, originSample);
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            decoder->pitch = value;
            decoder->processor->setTransposeFactor(value);
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            decoder->sampleRate = value;

            ma_uint32 originSample = decoder->decoder.outputSampleRate;
            ma_uint32 target = (ma_uint64)(value);

            ma_data_converter_set_rate(&decoder->decoder.converter, target, originSample);
            break;
        }

        default:
        {
            EST_EncoderSetError("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetAttribute(EST_ENCODER_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_PAN:
        {
            *value = ma_panner_get_pan(&decoder->panner);
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            ma_gainer_get_master_volume(&decoder->gainer, value);
            break;
        }

        case EST_ATTRIB_ENCODER_TEMPO:
        {
            *value = decoder->rate;
            break;
        }

        case EST_ATTRIB_ENCODER_PITCH:
        {
            *value = decoder->pitch;
            break;
        }

        case EST_ATTRIB_ENCODER_SAMPLERATE:
        {
            *value = decoder->sampleRate;
            break;
        }

        default:
        {
            EST_EncoderSetError("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetInfo(EST_ENCODER_HANDLE handle, est_encoder_info *info)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    info->channels = decoder->channels;
    info->pcmSize = decoder->numOfPcmProcessed;
    info->sampleRate = static_cast<int>(decoder->sampleRate);

    return EST_OK;
}