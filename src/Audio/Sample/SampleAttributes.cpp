#include "SampleInternal.h"

EST_RESULT EST_SampleSetAttribute(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_VOLUME:
            ma_gainer_set_master_volume(&it->gainer, value);
            break;
        case EST_ATTRIB_RATE:
            it->attributes.rate = value;
            // it->pitch->processor->setTransposeFactor(1.0f / value);
            ma_resampler_set_rate(&it->pitch->resampler, (ma_uint32)(value * (float)it->decoder.outputSampleRate), it->decoder.outputSampleRate);
            break;
        case EST_ATTRIB_PITCH:
            it->pitch->isPitched = value != 0.0f;
            break;
        case EST_ATTRIB_PAN:
            ma_panner_set_pan(&it->panner, value);
            break;
        case EST_ATTRIB_LOOPING:
            it->attributes.looping = value != 0.0f;
            break;
        default:
            EST_SetError("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_SampleGetAttribute(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    switch (attribute) {
        case EST_ATTRIB_VOLUME:
            ma_gainer_get_master_volume(&it->gainer, value);
            break;
        case EST_ATTRIB_RATE:
            *value = it->attributes.rate;
            break;
        case EST_ATTRIB_PITCH:
            *value = it->pitch->isPitched;
            break;
        case EST_ATTRIB_PAN:
            *value = ma_panner_get_pan(&it->panner);
            break;
        case EST_ATTRIB_LOOPING:
            *value = static_cast<float>(it->attributes.looping);
            break;
        default:
            EST_SetError("Invalid attribute");
            return EST_ERROR_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_SampleSlideAttribute(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, EST_ATTRIBUTE_FLAGS attribute, float value, float time)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (attribute == EST_ATTRIB_LOOPING) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (time <= 0) {
        return EST_SampleSetAttribute(device, handle, attribute, value);
    }

    float time_now = 0.0f;
    float time_end = time;

    float initialValue;
    EST_SampleGetAttribute(device, handle, attribute, &initialValue);

    auto prev = std::chrono::high_resolution_clock::now();
    while (time_now <= time_end) {
        auto now = std::chrono::high_resolution_clock::now();
        time_now += std::chrono::duration_cast<std::chrono::microseconds>(now - prev).count() / 1000.0f;

        float t = time_now / time_end;
        float v = initialValue + (value - initialValue) * t;
        v = std::clamp(v, initialValue, value);

        ::printf("Time: %.2f\n", time_now);

        EST_SampleSetAttribute(device, handle, attribute, v);
    }

    // Nothing perfect, this is to make sure
    EST_SampleSetAttribute(device, handle, attribute, value);

    return EST_OK;
}

EST_RESULT EST_SampleSlideAttributeAsync(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, EST_ATTRIBUTE_FLAGS attribute, float value, float time)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (attribute == EST_ATTRIB_LOOPING) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (time <= 0) {
        return EST_SampleSetAttribute(device, handle, attribute, value);
    }

    // Very cheap version, indeed
    std::thread([device, handle, attribute, value, time] {
        EST_SampleSlideAttribute(device, handle, attribute, value, time);
    }).detach();

    return EST_OK;
}

EST_RESULT EST_SampleSetVolume(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, float volume)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_gainer_set_master_volume(&it->gainer, volume);

    return EST_OK;
}

EST_RESULT EST_SampleSetCallback(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, est_audio_callback callback, void *userdata)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_AudioCallback callbackData = {};
    callbackData.callback = callback;
    callbackData.userdata = userdata;

    it->callbacks.push_back(callbackData);

    return EST_OK;
}

EST_RESULT EST_SampleSetGlobalCallback(EST_DEVICE_HANDLE devhandle, est_audio_callback callback, void *userdata)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    EST_AudioCallback callbackData = {};
    callbackData.callback = callback;
    callbackData.userdata = userdata;

    device->callbacks.push_back(callbackData);

    return EST_OK;
}