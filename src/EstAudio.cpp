#include <EstAudio.h>

#include "./third-party/miniaudio/miniaudio_decoders.h"

#define SIGNALSMITH_STRETCH_IMPLEMENTATION
#include "./third-party/signalsmith-stretch/signalsmith-stretch.h"

using namespace signalsmith::stretch;
#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

struct EST_AudioCallback
{
    est_audio_callback callback;
    void              *userdata;
};

struct EST_AudioDevice
{
    int channels = 0;

    ma_context context = {};
    ma_device  device = {};

    std::vector<float>             temporaryData;
    std::vector<float>             processingData;
    std::vector<EST_AudioCallback> callbacks;
};

struct EST_AudioResampler
{
    bool isInit = false;
    bool isPitched = false;

    ma_resampler                        resampler = {};
    std::shared_ptr<SignalsmithStretch> processor = {};
};

struct EST_Attribute
{
    float volume = 1.0f;
    float rate = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool  looping = false;
};

struct EST_AudioSample
{
    bool isInit = false;
    bool isPlaying = false;
    bool isAtEnd = false;
    bool isRemoved = false;

    EST_Attribute attributes = {};

    ma_decoder                          decoder = {};
    ma_panner                           panner = {};
    ma_gainer                           gainer = {};
    std::shared_ptr<EST_AudioResampler> pitch = {};

    std::vector<EST_AudioCallback> callbacks;
};

struct EST_AudioDestructor
{
    inline void operator()(EST_AudioSample *sample) const
    {
        if (!sample->isInit) {
            return;
        }

        ma_decoder_uninit(&sample->decoder);
        ma_gainer_uninit(&sample->gainer, nullptr);
    }
};

struct EST_ResamplerDestructor
{
    inline void operator()(EST_AudioResampler *sample) const
    {
        if (!sample->isInit) {
            return;
        }

        ma_resampler_uninit(&sample->resampler, nullptr);
        sample->processor->reset();
    }
};

namespace {
    static EHANDLE g_handleCounter = 0;

    std::shared_ptr<EST_AudioDevice>                              g_device;
    std::unordered_map<EHANDLE, std::shared_ptr<EST_AudioSample>> g_samples;

    std::string g_error;

    static std::shared_ptr<signalsmith::stretch::SignalsmithStretch> stretch;
    ma_uint32                                                        data_mix_pcm(EHANDLE handle, std::shared_ptr<EST_AudioSample> sample, float *pOutput, ma_uint32 frameCount)
    {
        int       channels = g_device->channels;
        ma_result result = MA_SUCCESS;

        auto &temp = g_device->processingData;
        auto &temp2 = g_device->temporaryData;

        uint32_t byteSize = channels * frameCount;

        std::fill(temp.begin(), temp.begin() + byteSize, 0.0f);
        std::fill(temp2.begin(), temp2.begin() + byteSize, 0.0f);

        ma_uint32 tempCapInFrames = temp.size() / channels;
        ma_uint32 totalFramesRead = 0;

        while (totalFramesRead < frameCount) {
            ma_uint64 iSample;
            ma_uint64 framesReadThisIteration;
            ma_uint64 totalFramesRemaining = frameCount - totalFramesRead;
            ma_uint64 framesToReadThisIteration = tempCapInFrames;
            ma_uint64 expectedToReadThisIteration = tempCapInFrames;
            if (framesToReadThisIteration > totalFramesRemaining) {
                framesToReadThisIteration = totalFramesRemaining;
                expectedToReadThisIteration = totalFramesRemaining;
            }

            if (sample->attributes.rate != 1.0f) {
                ma_resampler_get_required_input_frame_count(
                    &sample->pitch->resampler,
                    framesToReadThisIteration,
                    &framesToReadThisIteration);
            }

            result = ma_decoder_read_pcm_frames(&sample->decoder, &temp[0], framesToReadThisIteration, &framesReadThisIteration);
            if (result != MA_SUCCESS || framesReadThisIteration == 0) {
                break;
            }

            if (sample->attributes.rate != 1.0f) {
                if (!sample->pitch->isPitched) {
                    sample->pitch->processor->process(temp, framesReadThisIteration, temp2, expectedToReadThisIteration);

                    framesReadThisIteration = expectedToReadThisIteration;

                    std::copy(temp2.begin(), temp2.begin() + framesReadThisIteration * channels, temp.begin());
                } else {
                    result = ma_resampler_process_pcm_frames(&sample->pitch->resampler, &temp[0], &framesReadThisIteration, &temp2[0], &expectedToReadThisIteration);

                    if (result != MA_SUCCESS) {
                        break;
                    }

                    framesReadThisIteration = expectedToReadThisIteration;

                    std::copy(temp2.begin(), temp2.begin() + framesReadThisIteration * channels, temp.begin());
                }
            }

            result = ma_panner_process_pcm_frames(&sample->panner, &temp2[0], &temp[0], framesReadThisIteration);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_gainer_process_pcm_frames(&sample->gainer, &temp[0], &temp2[0], framesReadThisIteration);
            if (result != MA_SUCCESS) {
                break;
            }

            /* Mix the frames together. */
            for (iSample = 0; iSample < framesReadThisIteration * channels; ++iSample) {
                pOutput[totalFramesRead * channels + iSample] += temp[iSample];
            }

            totalFramesRead += (ma_uint32)framesReadThisIteration;

            if (framesReadThisIteration < (ma_uint32)framesToReadThisIteration) {
                break; /* Reached EOF. */
            }
        }

        if (sample->callbacks.size()) {
            for (auto &it : g_device->callbacks) {
                it.callback(handle, it.userdata, pOutput, frameCount);
            }
        }

        return totalFramesRead;
    }

    template <typename ContainerT, typename PredicateT>
    void erase_if_map(ContainerT &items, const PredicateT &predicate)
    {
        for (auto it = items.begin(); it != items.end();) {
            if (predicate(*it))
                it = items.erase(it);
            else
                ++it;
        }
    }

    void data_callback(ma_device *pObject, void *pOutput, const void *pInput, ma_uint32 frameCount)
    {
        if (!g_device) {
            return;
        }

        float *pOutputFloat = reinterpret_cast<float *>(pOutput);

        for (auto &[handle, sample] : g_samples) {
            if (sample->isPlaying) {
                ma_uint32 pcmReaded = data_mix_pcm(handle, sample, pOutputFloat, frameCount);
                if (pcmReaded < frameCount) {
                    if (sample->attributes.looping) {
                        ma_decoder_seek_to_pcm_frame(&sample->decoder, 0);
                    } else {
                        sample->isAtEnd = false;
                        sample->isPlaying = false;
                    }
                }
            }
        }

        erase_if_map(
            g_samples,
            [&](std::pair<EHANDLE, std::shared_ptr<EST_AudioSample>> sample) {
                return sample.second->isRemoved;
            });

        if (g_device->callbacks.size()) {
            for (auto &it : g_device->callbacks) {
                it.callback(-1, it.userdata, pOutputFloat, frameCount);
            }
        }
    }
} // namespace

EST_RESULT EST_DeviceInit(int sampleRate, int channels)
{
    if (channels != 2) {
        EST_SetError("Only stereo supported right now");
        return EST_INVALID_ARGUMENT;
    }

    if (g_device) {
        EST_SetError("Already intialized");
        return EST_INVALID_OPERATION;
    }

    try {
        g_device = std::make_shared<EST_AudioDevice>();
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_OUT_OF_MEMORY;
    }

    g_device->channels = channels;

    if (ma_context_init(NULL, 0, NULL, &g_device->context) != MA_SUCCESS) {
        EST_SetError("Failed to initialize audio context");
        return EST_INVALID_OPERATION;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = channels;
    config.sampleRate = sampleRate;
    config.dataCallback = data_callback;
    config.periodSizeInMilliseconds = 0;
    config.pUserData = NULL;

    if (ma_device_init(&g_device->context, &config, &g_device->device) != MA_SUCCESS) {
        EST_SetError("Failed to initialize audio device");
        ma_context_uninit(&g_device->context);
        return EST_INVALID_OPERATION;
    }

    if (ma_device_start(&g_device->device) != MA_SUCCESS) {
        EST_SetError("Failed to start audio device");
        ma_device_uninit(&g_device->device);
        ma_context_uninit(&g_device->context);
        return EST_INVALID_OPERATION;
    }

    g_device->temporaryData.resize(4095 * channels);
    g_device->processingData.resize(4095 * channels);

    return EST_OK;
}

EST_RESULT EST_DeviceShutdown()
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    for (auto &it : g_samples) {
        EST_SampleUnload(it.first);
    }

    while (g_samples.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    ma_device_uninit(&g_device->device);
    ma_context_uninit(&g_device->context);

    g_device.reset();

    return EST_OK;
}

EST_RESULT InternalInit(std::shared_ptr<EST_AudioSample> sample, EHANDLE *handle)
{
    ma_panner_config pannerConfig = ma_panner_config_init(sample->decoder.outputFormat, sample->decoder.outputChannels);
    if (ma_panner_init(&pannerConfig, &sample->panner) != MA_SUCCESS) {
        EST_SetError("Failed to initialize panner");
        return EST_INVALID_ARGUMENT;
    }

    ma_gainer_config gainerConfig = ma_gainer_config_init(sample->decoder.outputChannels, 0);
    if (ma_gainer_init(&gainerConfig, nullptr, &sample->gainer) != MA_SUCCESS) {
        EST_SetError("Failed to initialize gainer");
        return EST_INVALID_ARGUMENT;
    }

    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        sample->decoder.outputFormat,
        sample->decoder.outputChannels,
        g_device->device.sampleRate,
        g_device->device.sampleRate,
        ma_resample_algorithm_linear);

    std::shared_ptr<EST_AudioResampler> pitch;

    try {
        pitch = std::shared_ptr<EST_AudioResampler>(new EST_AudioResampler, EST_ResamplerDestructor{});
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_OUT_OF_MEMORY;
    }

    pitch->processor = std::make_shared<SignalsmithStretch>();
    pitch->processor->presetDefault(sample->decoder.outputChannels, sample->decoder.outputSampleRate);

    if (ma_resampler_init(&resamplerConfig, nullptr, &pitch->resampler) != MA_SUCCESS) {
        EST_SetError("Failed to initialize gainer");
        return EST_INVALID_ARGUMENT;
    }

    pitch->isInit = true;
    sample->isInit = true;

    sample->pitch = pitch;

    EHANDLE id = g_handleCounter++;

    g_samples[id] = sample;
    *handle = id;

    return EST_OK;
}

EST_RESULT EST_SampleLoad(const char *path, EHANDLE *handle)
{
    if (!path) {
        EST_SetError("'path' is nullptr");
        return EST_ERROR;
    }

    std::shared_ptr<EST_AudioSample> sample;

    try {
        sample = std::shared_ptr<EST_AudioSample>(new EST_AudioSample, EST_AudioDestructor{});
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_OUT_OF_MEMORY;
    }

    ma_result         result = MA_SUCCESS;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    if (ma_decoder_init_file(path, &config, &sample->decoder) != MA_SUCCESS) {
        EST_SetError("Failed to load audio file");
        return EST_INVALID_ARGUMENT;
    }

    return InternalInit(sample, handle);
}

EST_RESULT EST_SampleLoadMemory(const void *data, int size, EHANDLE *handle)
{
    if (!data) {
        EST_SetError("'data' is nullptr");
        return EST_ERROR;
    }

    std::shared_ptr<EST_AudioSample> sample;

    try {
        sample = std::shared_ptr<EST_AudioSample>(new EST_AudioSample, EST_AudioDestructor{});
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_OUT_OF_MEMORY;
    }

    ma_result         result = MA_SUCCESS;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    if (ma_decoder_init_memory(data, size, &config, &sample->decoder) != MA_SUCCESS) {
        EST_SetError("Failed to load audio file");
        return EST_INVALID_ARGUMENT;
    }

    return InternalInit(sample, handle);
}

std::shared_ptr<EST_AudioSample> GetSample(EHANDLE handle)
{
    auto it = g_samples.find(handle);
    if (it == g_samples.end()) {
        return nullptr;
    }

    return it->second;
}

EST_RESULT EST_SampleUnload(EHANDLE handle)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR;
    }

    it->isRemoved = true;

    return EST_OK;
}

EST_RESULT EST_SamplePlay(EHANDLE handle)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
    }

    it->isAtEnd = false;
    it->isPlaying = true;
    ma_decoder_seek_to_pcm_frame(&it->decoder, 0);

    return EST_OK;
}

EST_RESULT EST_SampleStop(EHANDLE handle)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
    }

    it->isPlaying = false;
    ma_decoder_seek_to_pcm_frame(&it->decoder, 0);

    return EST_OK;
}

EST_RESULT EST_SampleGetStatus(EHANDLE handle, enum EST_STATUS *value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
    }

    if (it->isPlaying) {
        *value = EST_STATUS_PLAYING;
    } else {
        if (it->isAtEnd) {
            *value = EST_STATUS_AT_END;
        } else {
            *value = EST_STATUS_IDLE;
        }
    }

    return EST_OK;
}

EST_RESULT EST_SampleSetAttribute(EHANDLE handle, enum EST_ATTRIBUTE attribute, float value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
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
            it->pitch->isPitched = value != 0.0f || static_cast<EST_BOOL>(value) == EST_TRUE;
            break;
        case EST_ATTRIB_PAN:
            ma_panner_set_pan(&it->panner, value);
            break;
        case EST_ATTRIB_LOOPING:
            it->attributes.looping = value != 0.0f || static_cast<EST_BOOL>(value) == EST_TRUE;
            break;
        default:
            EST_SetError("Invalid attribute");
            return EST_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_SampleGetAttribute(EHANDLE handle, enum EST_ATTRIBUTE attribute, float *value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
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
            *value = static_cast<EST_BOOL>(it->attributes.looping);
            break;
        default:
            EST_SetError("Invalid attribute");
            return EST_INVALID_ARGUMENT;
    }

    return EST_OK;
}

EST_RESULT EST_SampleSetVolume(EHANDLE handle, float volume)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
    }

    ma_gainer_set_master_volume(&it->gainer, volume);

    return EST_OK;
}

EST_RESULT EST_SampleSetCallback(EHANDLE handle, est_audio_callback callback, void *userdata)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_INVALID_ARGUMENT;
    }

    EST_AudioCallback callbackData = {};
    callbackData.callback = callback;
    callbackData.userdata = userdata;

    it->callbacks.push_back(callbackData);

    return EST_OK;
}

EST_RESULT EST_SampleSetGlobalCallback(est_audio_callback callback, void *userdata)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_INVALID_STATE;
    }

    EST_AudioCallback callbackData = {};
    callbackData.callback = callback;
    callbackData.userdata = userdata;

    g_device->callbacks.push_back(callbackData);

    return EST_OK;
}

const char *EST_GetError()
{
    if (g_error.empty()) {
        return nullptr;
    }

    return g_error.c_str();
}

void EST_SetError(const char *error)
{
    g_error = error;
}