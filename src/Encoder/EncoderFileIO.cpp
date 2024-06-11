#include "EncoderInternal.h"

namespace {
    static std::string gErrorString;
}

EST_RESULT InternalInit(EST_Encoder *sample, ma_format format, int channels, int sampleRate, EST_ENCODER_HANDLE *handle)
{
    ma_panner_config pannerConfig = ma_panner_config_init(format, channels);
    if (ma_panner_init(&pannerConfig, &sample->panner) != MA_SUCCESS) {
        EST_EncoderSetError("Failed to initialize panner");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_gainer_config gainerConfig = ma_gainer_config_init(channels, 0);
    if (ma_gainer_init(&gainerConfig, nullptr, &sample->gainer) != MA_SUCCESS) {
        EST_EncoderSetError("Failed to initialize gainer");
        return EST_ERROR_INVALID_ARGUMENT;
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
        EST_EncoderSetError("Failed to initialize gainer");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_channel_converter_config chConfig = ma_channel_converter_config_init(
        format,                       // Sample format
        channels,                     // Input channels
        NULL,                         // Input channel map
        sample->channels,             // Output channels
        NULL,                         // Output channel map
        ma_channel_mix_mode_default); // The mixing algorithm to use when combining channels.

    if (ma_channel_converter_init(&chConfig, nullptr, &sample->converter)) {
        EST_EncoderSetError("Failed to initialize channel converter");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    sample->data.reserve(4095 * channels);
    *handle = reinterpret_cast<EST_ENCODER_HANDLE>(sample);

    return EST_OK;
}

EST_RESULT EST_EncoderLoad(const char *path, est_encoder_callback callback, enum EST_DECODER_FLAGS flags, EST_ENCODER_HANDLE *decoder)
{
    if (!path) {
        EST_EncoderSetError("Path is not defined");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Encoder *instance = new EST_Encoder;
    if (!instance) {
        return EST_ERROR_OUT_OF_MEMORY;
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
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (flags & EST_DECODER_MONO) {
        instance->channels = 1;
    }

    instance->flags = flags;
    instance->callback = callback;
    return InternalInit(instance,
                        instance->decoder.outputFormat,
                        instance->decoder.outputChannels,
                        instance->decoder.outputSampleRate,
                        decoder);
}

EST_RESULT EST_EncoderLoadMemory(const void *data, int size, est_encoder_callback callback, enum EST_DECODER_FLAGS flags, EST_ENCODER_HANDLE *decoder)
{
    if (!data || size == 0) {
        EST_EncoderSetError("Path is not defined");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Encoder *instance = new EST_Encoder;
    if (!instance) {
        return EST_ERROR_OUT_OF_MEMORY;
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
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (flags & EST_DECODER_MONO) {
        instance->channels = 1;
    }

    instance->flags = flags;
    instance->callback = callback;
    return InternalInit(instance,
                        instance->decoder.outputFormat,
                        instance->decoder.outputChannels,
                        instance->decoder.outputSampleRate,
                        decoder);
}

EST_RESULT EST_EncoderFree(EST_ENCODER_HANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_uninit(&decoder->decoder);
    ma_gainer_uninit(&decoder->gainer, nullptr);
    ma_channel_converter_uninit(&decoder->converter, nullptr);

    delete decoder;
    return EST_OK;
}

EST_RESULT EST_EncoderSetError(const char *error)
{
    gErrorString = error;
    return EST_OK;
}

const char *EST_EncoderGetError()
{
    return gErrorString.c_str();
}