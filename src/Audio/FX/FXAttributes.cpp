#include "../Internal.h"
#include "../../Utils/IO.h"
#include "../../Encoder/EncoderInternal.h"

EST_RESULT EST_FXSetAttribute(struct EST_FX *fx, est_attribute_value *value)
{
    if (!fx) {
        return EST_ERROR;
    }

    if (!value) {
        return EST_ERROR;
    }

    switch (value->attribute) {
        case EST_ATTRIB_FX_PITCH:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.pitch = value->fValue;
            fx->processor->setTransposeFactor(value->fValue);
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            if (value->type != EST_ATTRIB_VAL_FLOAT) {
                return EST_ERROR;
            }

            fx->attributes.tempo = value->fValue;

            ma_uint32 originSample = fx->sampleRate;
            ma_uint32 target = (ma_uint64)(originSample * value->fValue);

            ma_resampler_set_rate(&fx->resampler, target, originSample);
            break;
        }
    }

    return EST_OK;
}

EST_RESULT EST_FXGetAttribute(struct EST_FX *fx, est_attribute_value *value)
{
    if (!fx) {
        EST_ErrorSetMessage("EST_FXGetAttribute: Invalid FX");
        return EST_ERROR;
    }

    if (!value) {
        EST_ErrorSetMessage("EST_FXGetAttribute: Invalid value");
        return EST_ERROR;
    }

    switch (value->attribute) {
        case EST_ATTRIB_FX_PITCH:
        {
            value->fValue = fx->attributes.pitch;
            break;
        }

        case EST_ATTRIB_FX_TEMPO:
        {
            value->fValue = fx->attributes.tempo;
            break;
        }

        default:
        {
            EST_ErrorSetMessage("EST_FXGetAttribute: Invalid attribute");
            return EST_ERROR;
        }
    }

    return EST_OK;
}