#include "SampleInternal.h"

EST_RESULT InternalInit(EST_DEVICE_HANDLE devhandle, std::shared_ptr<EST_AudioSample> sample, ma_format format, int channels, int sampleRate, EST_AUDIO_HANDLE *handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

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
        device->device.sampleRate,
        device->device.sampleRate,
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
        format,                       // Sample format
        channels,                     // Input channels
        NULL,                         // Input channel map
        device->channels,             // Output channels
        NULL,                         // Output channel map
        ma_channel_mix_mode_default); // The mixing algorithm to use when combining channels.

    if (ma_channel_converter_init(&chConfig, nullptr, &sample->converter)) {
        EST_SetError("Failed to initialize channel converter");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    pitch->isInit = true;
    sample->isInit = true;
    sample->channels = channels;
    sample->pitch = pitch;

    std::lock_guard<std::mutex> lock(*device->mutex.get());

    EST_AUDIO_HANDLE id = device->HandleCounter++;

    device->samples[id] = sample;
    *handle = id;

    return EST_OK;
}

EST_RESULT EST_SampleLoad(EST_DEVICE_HANDLE devhandle, const char *path, EST_AUDIO_HANDLE *handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

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

    return InternalInit(device, sample, sample->decoder.outputFormat, sample->decoder.outputChannels, sample->decoder.outputSampleRate, handle);
}

EST_RESULT EST_SampleLoadMemory(EST_DEVICE_HANDLE devhandle, const void *data, int size, EST_AUDIO_HANDLE *handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

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

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
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

    return InternalInit(device, sample, sample->decoder.outputFormat, sample->decoder.outputChannels, sample->decoder.outputSampleRate, handle);
}

EST_RESULT EST_SampleLoadRawPCM(EST_DEVICE_HANDLE devhandle, const void *data, int pcmSize, int channels, int sampleRate, EST_AUDIO_HANDLE *handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!data) {
        return EST_ERROR_INVALID_DATA;
    }

    int expectedDataSize = pcmSize * channels;

    std::shared_ptr<EST_RawAudio>    rawAudio;
    std::shared_ptr<EST_AudioSample> sample;
    try {
        rawAudio = std::make_shared<EST_RawAudio>();
        rawAudio->PCMData.resize(expectedDataSize);

        sample = std::shared_ptr<EST_AudioSample>(new EST_AudioSample, EST_AudioDestructor{});
    } catch (const std::bad_alloc &) {
        EST_SetError("Out of memory!");
        return EST_ERROR_OUT_OF_MEMORY;
    }

    // memcpy(&rawAudio->PCMData[0], data, expectedDataSize * sizeof(float));

    const float *pFloatData = static_cast<const float *>(data);
    std::copy(pFloatData, pFloatData + expectedDataSize, &rawAudio->PCMData[0]);
    rawAudio->PCMSize = pcmSize;

    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_f32,
        channels,
        pcmSize,
        &rawAudio->PCMData[0],
        nullptr);

    auto result = ma_audio_buffer_init(&config, &rawAudio->decoder);
    if (result != MA_SUCCESS) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    sample->rawAudio = rawAudio;

    return InternalInit(device, sample, ma_format_f32, channels, sampleRate, handle);
}

EST_RESULT EST_SampleFree(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR;
    }

    it->isRemoved = true;

    return EST_OK;
}