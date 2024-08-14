/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "../Internal.h"
#include "../../Utils/IO.h"
#include "../../Encoder/EncoderInternal.h"

EST_RESULT EST_SampleSetAttribute(EST_Sample *sample, est_attribute_value *value)
{
    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(sample);
    if (unknown->type != EST_UNKNOWN_SAMPLE) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_SAMPLERATE:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Invalid type for rate");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            sample->attributes.samplerate = value->fValue;
            break;
        }

        case EST_ATTRIB_PAN:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Invalid type for pan");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            sample->attributes.pan = value->fValue;
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Invalid type for volume");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            sample->attributes.volume = value->fValue;
            break;
        }

        case EST_ATTRIB_FX_PITCH:
        {
            auto fx = sample->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Sample is not FX Channel!");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.pitch = value->fValue;
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            auto fx = sample->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Sample is not FX Channel!");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.tempo = value->fValue;
            break;
        }

        default:
            EST_ErrorSetMessage("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_SampleGetAttribute(EST_Sample *sample, est_attribute_value *value)
{
    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(sample);
    if (unknown->type != EST_UNKNOWN_SAMPLE) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (value->attribute) {
        case EST_ATTRIB_SAMPLERATE:
        {
            value->fValue = sample->attributes.samplerate;
            break;
        }

        case EST_ATTRIB_PAN:
        {
            value->fValue = sample->attributes.pan;
            break;
        }

        case EST_ATTRIB_VOLUME:
        {
            value->fValue = sample->attributes.volume;
            break;
        }

        case EST_ATTRIB_FX_PITCH:
        {
            auto fx = sample->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Sample is not FX Channel!");
                return EST_ERROR_INVALID_ARGUMENT;
            }

            value->fValue = fx->attributes.pitch;
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            auto fx = sample->fx;
            if (fx == nullptr) {
                EST_ErrorSetMessage("EST_SampleSetAttribute: Sample is not FX Channel!");
                return EST_ERROR_INVALID_ARGUMENT;
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