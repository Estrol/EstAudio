#include "EncoderInternal.h"

namespace {
    static std::string gErrorString;
}

EST_Encoder *InternalInit(EST_Encoder *sample, ma_format format, int channels, int sampleRate)
{
    ma_panner_config pannerConfig = ma_panner_config_init(format, channels);
    if (ma_panner_init(&pannerConfig, &sample->panner) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize panner");
        return nullptr;
    }

    ma_gainer_config gainerConfig = ma_gainer_config_init(channels, 0);
    if (ma_gainer_init(&gainerConfig, nullptr, &sample->gainer) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize gainer");
        return nullptr;
    }

    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        format,
        channels,
        sampleRate,
        sampleRate,
        ma_resample_algorithm_linear);

    sample->processor = std::make_shared<SignalsmithStretch>();
    sample->processor->configure(channels, sampleRate, sampleRate);
    sample->sampleRate = static_cast<float>(sampleRate);

    if (ma_resampler_init(&resamplerConfig, nullptr, &sample->calculator) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize gainer");
        return nullptr;
    }

    ma_channel_converter_config chConfig = ma_channel_converter_config_init(
        format,                       // Sample format
        channels,                     // Input channels
        NULL,                         // Input channel map
        sample->channels,             // Output channels
        NULL,                         // Output channel map
        ma_channel_mix_mode_default); // The mixing algorithm to use when combining channels.

    if (ma_channel_converter_init(&chConfig, nullptr, &sample->converter)) {
        EST_ErrorSetMessage("Failed to initialize channel converter");
        return nullptr;
    }

    sample->data.reserve(4095 * channels);

    return sample;
}

EST_Encoder *EST_EncoderLoad(const char *path, EST_ENCODER_CALLBACK callback, enum EST_DECODER_FLAGS flags)
{
    if (!path) {
        EST_ErrorSetMessage("Path is not defined");
        return nullptr;
    }

    EST_Encoder *instance = new EST_Encoder;
    if (!instance) {
        return nullptr;
    }

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);
    config.allowDynamicSampleRate = MA_TRUE;

    auto result = ma_decoder_init_file(path, &config, &instance->decoder);
    if (result != MA_SUCCESS) {
        delete instance;
        return nullptr;
    }

    if (flags & EST_DECODER_MONO) {
        instance->channels = 1;
    }

    instance->flags = flags;
    instance->callback = callback;
    return InternalInit(instance,
                        instance->decoder.outputFormat,
                        instance->decoder.outputChannels,
                        instance->decoder.outputSampleRate);
}

EST_Encoder *EST_EncoderLoadMemory(const void *data, int size, EST_ENCODER_CALLBACK callback, enum EST_DECODER_FLAGS flags)
{
    if (!data || size == 0) {
        EST_ErrorSetMessage("Path is not defined");
        return nullptr;
    }

    EST_Encoder *instance = new EST_Encoder;
    if (!instance) {
        return nullptr;
    }

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);
    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);
    config.allowDynamicSampleRate = MA_TRUE;

    auto result = ma_decoder_init_memory(data, size, &config, &instance->decoder);
    if (result != MA_SUCCESS) {

        delete instance;
        return nullptr;
    }

    if (flags & EST_DECODER_MONO) {
        instance->channels = 1;
    }

    instance->flags = flags;
    instance->callback = callback;
    return InternalInit(instance,
                        instance->decoder.outputFormat,
                        instance->decoder.outputChannels,
                        instance->decoder.outputSampleRate);
}

EST_RESULT EST_EncoderFree(EST_Encoder *handle)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_ENCODER) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_ENCODER));
        EST_ErrorSetMessage(msg.c_str());
        return EST_ERROR_INVALID_ARGUMENT;
    }

    for (int i = 0; i < handle->owned_channels.size(); i++) {
        EST_ChannelStop(handle->owned_channels[i].get());
        EST_ChannelFree(handle->owned_channels[i].get());
    }

    ma_decoder_uninit(&handle->decoder);
    ma_gainer_uninit(&handle->gainer, nullptr);
    ma_channel_converter_uninit(&handle->converter, nullptr);

    unknown->type = EST_UNKNOWN_NONE;

    delete handle;
    return EST_OK;
}