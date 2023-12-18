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

struct EST_RawAudio
{
    ma_audio_buffer decoder = {};

    std::vector<float> PCMData;
    int PCMSize = 0;
};

struct EST_AudioSample
{
    int channels = 0;

    bool isInit = false;
    bool isPlaying = false;
    bool isAtEnd = false;
    bool isRemoved = false;

    EST_Attribute attributes = {};
    std::shared_ptr<EST_RawAudio> rawAudio;

    ma_decoder                          decoder = {};
    ma_panner                           panner = {};
    ma_gainer                           gainer = {};
    ma_channel_converter converter = {};
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

        if (sample->rawAudio) {
            ma_audio_buffer_uninit(&sample->rawAudio->decoder);
        }
        else {
            ma_decoder_uninit(&sample->decoder);
        }

        ma_channel_converter_uninit(&sample->converter, nullptr);
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
    constexpr int kMaxChannels = 2;
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

        ma_uint32 tempCapInFrames = static_cast<int>(temp.size()) / channels;
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

            if (sample->rawAudio) {
                framesReadThisIteration = ma_audio_buffer_read_pcm_frames(&sample->rawAudio->decoder, &temp[0], framesToReadThisIteration, MA_FALSE);
                if (framesReadThisIteration == 0) {
                    break;
                }
            }
            else {
                result = ma_decoder_read_pcm_frames(&sample->decoder, &temp[0], framesToReadThisIteration, &framesReadThisIteration);
                if (result != MA_SUCCESS || framesReadThisIteration == 0) {
                    break;
                }
            }

            if (sample->channels != g_device->channels) {
                ma_channel_converter_process_pcm_frames(&sample->converter, &temp2[0], &temp[0], framesReadThisIteration);

                std::fill(temp.begin(), temp.begin() + byteSize, 0.0f);
                std::copy(&temp2[0], &temp2[0] + framesReadThisIteration * g_device->channels, &temp[0]);
            }

            if (sample->attributes.rate != 1.0f) {
                result = ma_resampler_process_pcm_frames(&sample->pitch->resampler, &temp[0], &framesReadThisIteration, &temp2[0], &expectedToReadThisIteration);

                if (result != MA_SUCCESS) {
                    break;
                }

                framesReadThisIteration = expectedToReadThisIteration;

                if (!sample->pitch->isPitched) {
                    sample->pitch->processor->process(
                        temp2, 
                        static_cast<int>(framesReadThisIteration), 
                        temp, 
                        static_cast<int>(framesReadThisIteration));
                } else {
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
                pOutput[totalFramesRead * channels + iSample] += temp[iSample]; //std::clamp(temp[iSample], -1.0f, 1.0f);
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
                        sample->isAtEnd = true;
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
                it.callback((EHANDLE)INVALID_HANDLE, it.userdata, pOutputFloat, frameCount);
            }
        }

        int frameCountInBytes = frameCount * g_device->channels;
        for (int i = 0; i < frameCountInBytes; i++) {
            pOutputFloat[i] = std::clamp(pOutputFloat[i], -1.0f, 1.0f);
        }

        (void)pObject;
        (void)pInput;
    }
} // namespace

EST_RESULT EST_DeviceInit(int sampleRate, enum EST_DEVICE_FLAGS flags)
{
    if (g_device) {
        EST_SetError("Already intialized");
        return EST_ERROR_INVALID_OPERATION;
    }

    try {
        g_device = std::make_shared<EST_AudioDevice>();
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_ERROR_OUT_OF_MEMORY;
    }

    int channels = 2;
    if (flags & EST_DEVICE_MONO) {
        channels = 1;
    }

    g_device->channels = channels;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = channels;
    config.sampleRate = sampleRate;
    config.dataCallback = data_callback;
    config.periodSizeInMilliseconds = 0;
    config.pUserData = NULL;

    auto result = ma_device_init(NULL, &config, &g_device->device);
    if (result != MA_SUCCESS) {
        EST_SetError("Failed to initialize audio device");
        ma_context_uninit(&g_device->context);
        return EST_ERROR_INVALID_OPERATION;
    }

    if (ma_device_start(&g_device->device) != MA_SUCCESS) {
        EST_SetError("Failed to start audio device");
        ma_device_uninit(&g_device->device);
        ma_context_uninit(&g_device->context);
        return EST_ERROR_INVALID_OPERATION;
    }

    g_device->temporaryData.resize(4095 * kMaxChannels);
    g_device->processingData.resize(4095 * kMaxChannels);

    return EST_OK;
}

EST_RESULT EST_GetInfo(est_device_info* info)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    info->channels = g_device->channels;
    info->deviceIndex = -1;
    info->sampleRate = g_device->device.sampleRate;
    info->flags = EST_DEVICE_UNKNOWN;

    return EST_OK;
}

EST_RESULT EST_DeviceFree()
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    for (auto &it : g_samples) {
        EST_SampleFree(it.first);
    }

    while (g_samples.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    ma_device_uninit(&g_device->device);
    //ma_context_uninit(&g_device->context);

    g_device.reset();

    return EST_OK;
}

EST_RESULT InternalInit(std::shared_ptr<EST_AudioSample> sample, ma_format format, int channels, int sampleRate, EHANDLE *handle)
{
    ma_panner_config pannerConfig = ma_panner_config_init(format, channels);
    if (ma_panner_init(&pannerConfig, &sample->panner) != MA_SUCCESS) {
        EST_SetError("Failed to initialize panner");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_gainer_config gainerConfig = ma_gainer_config_init(channels, 0);
    if (ma_gainer_init(&gainerConfig, nullptr, &sample->gainer) != MA_SUCCESS) {
        EST_SetError("Failed to initialize gainer");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        format,
        channels,
        g_device->device.sampleRate,
        g_device->device.sampleRate,
        ma_resample_algorithm_linear);

    std::shared_ptr<EST_AudioResampler> pitch;

    try {
        pitch = std::shared_ptr<EST_AudioResampler>(new EST_AudioResampler, EST_ResamplerDestructor{});
    } catch (std::bad_alloc &alloc) {
        EST_SetError(alloc.what());
        return EST_ERROR_OUT_OF_MEMORY;
    }

    pitch->processor = std::make_shared<SignalsmithStretch>();
    pitch->processor->presetCheaper(channels, static_cast<float>(sampleRate));

    if (ma_resampler_init(&resamplerConfig, nullptr, &pitch->resampler) != MA_SUCCESS) {
        EST_SetError("Failed to initialize gainer");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_channel_converter_config chConfig = ma_channel_converter_config_init(
        format,                         // Sample format
        channels,                       // Input channels
        NULL,                           // Input channel map
        g_device->channels,             // Output channels
        NULL,                           // Output channel map
        ma_channel_mix_mode_default);   // The mixing algorithm to use when combining channels.

    if (ma_channel_converter_init(&chConfig, nullptr, &sample->converter)) {
        EST_SetError("Failed to initialize channel converter");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    pitch->isInit = true;
    sample->isInit = true;
    sample->channels = channels;
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
        return EST_ERROR_OUT_OF_MEMORY;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);

    if (ma_decoder_init_file(path, &config, &sample->decoder) != MA_SUCCESS) {
        EST_SetError("Failed to load audio file");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    return InternalInit(sample, sample->decoder.outputFormat, sample->decoder.outputChannels, sample->decoder.outputSampleRate, handle);
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
        return EST_ERROR_OUT_OF_MEMORY;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_decoding_backend_vtable* pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);

    if (ma_decoder_init_memory(data, size, &config, &sample->decoder) != MA_SUCCESS) {
        EST_SetError("Failed to load audio file");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    return InternalInit(sample, sample->decoder.outputFormat, sample->decoder.outputChannels, sample->decoder.outputSampleRate, handle);
}

EST_RESULT EST_SampleLoadRawPCM(const void* data, int pcmSize, int channels, int sampleRate, EHANDLE* handle)
{
    if (!data) {
        return EST_ERROR_INVALID_DATA;
    }

    int expectedDataSize = pcmSize * channels;

    std::shared_ptr<EST_RawAudio> rawAudio;
    std::shared_ptr<EST_AudioSample> sample;
    try {
        rawAudio = std::make_shared<EST_RawAudio>();
        rawAudio->PCMData.resize(expectedDataSize);

        sample = std::shared_ptr<EST_AudioSample>(new EST_AudioSample, EST_AudioDestructor{});
    }
    catch (const std::bad_alloc&)
    {
        EST_SetError("Out of memory!");
        return EST_ERROR_OUT_OF_MEMORY;
    }

    //memcpy(&rawAudio->PCMData[0], data, expectedDataSize * sizeof(float));

    const float* pFloatData = static_cast<const float*>(data);
    std::copy(pFloatData, pFloatData + expectedDataSize, &rawAudio->PCMData[0]);
    rawAudio->PCMSize = pcmSize;

    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_f32,
        channels,
        pcmSize,
        &rawAudio->PCMData[0],
        nullptr
    );

    auto result = ma_audio_buffer_init(&config, &rawAudio->decoder);
    if (result != MA_SUCCESS) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    sample->rawAudio = rawAudio;

    return InternalInit(sample, ma_format_f32, channels, sampleRate, handle);
}

std::shared_ptr<EST_AudioSample> GetSample(EHANDLE handle)
{
    auto it = g_samples.find(handle);
    if (it == g_samples.end()) {
        return nullptr;
    }

    return it->second;
}

EST_RESULT EST_SampleFree(EHANDLE handle)
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
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    it->isAtEnd = false;
    it->isPlaying = true;
    it->pitch->processor->reset();

    if (it->rawAudio) {
        ma_audio_buffer_seek_to_pcm_frame(&it->rawAudio->decoder, 0);
    }
    else {
        ma_decoder_seek_to_pcm_frame(&it->decoder, 0);
    }

    return EST_OK;
}

EST_RESULT EST_SampleStop(EHANDLE handle)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    it->isPlaying = false;
    if (it->rawAudio) {
        ma_audio_buffer_seek_to_pcm_frame(&it->rawAudio->decoder, 0);
    }
    else {
        ma_decoder_seek_to_pcm_frame(&it->decoder, 0);
    }

    return EST_OK;
}

EST_RESULT EST_SampleGetStatus(EHANDLE handle, enum EST_STATUS *value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
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

EST_RESULT EST_SampleSetAttribute(EHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
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
            //it->pitch->processor->setTransposeFactor(1.0f / value);
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

EST_RESULT EST_SampleGetAttribute(EHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
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

EST_RESULT EST_SampleSetVolume(EHANDLE handle, float volume)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_gainer_set_master_volume(&it->gainer, volume);

    return EST_OK;
}

EST_RESULT EST_SampleSetCallback(EHANDLE handle, est_audio_callback callback, void *userdata)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(handle);
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

EST_RESULT EST_SampleSetGlobalCallback(est_audio_callback callback, void *userdata)
{
    if (!g_device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
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