#include "EstEncoder.h"
#include "./third-party/signalsmith-stretch/signalsmith-stretch.h"
#include "EstAudio.h"
#include "third-party/miniaudio/miniaudio_decoders.h"
#include <algorithm>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace signalsmith::stretch;
constexpr int kESTEncoderSignature = 255 * 0xff;

struct EST_Encoder
{
    const int Signature = kESTEncoderSignature;
    void     *userData = NULL;

    est_encoder_callback callback = NULL;
    std::vector<float>   data;

    ma_decoder           decoder = {};
    ma_gainer            gainer = {};
    ma_panner            panner = {};
    ma_resampler         calculator = {};
    ma_channel_converter converter = {};

    float             rate = 1.0f;
    float             pitch = 1.0f;
    float             sampleRate = 44100;
    int               numOfPcmProcessed = 0;
    int               channels = 2;
    EST_DECODER_FLAGS flags = EST_DECODER_UNKNOWN;

    std::shared_ptr<SignalsmithStretch> processor;
};

EST_RESULT InternalInit(EST_Encoder *sample, ma_format format, int channels, int sampleRate, ECHANDLE *handle)
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
        sampleRate,
        sampleRate,
        ma_resample_algorithm_linear);

    sample->processor = std::make_shared<SignalsmithStretch>();
    sample->processor->configure(channels, sampleRate, sampleRate);
    sample->sampleRate = static_cast<float>(sampleRate);

    if (ma_resampler_init(&resamplerConfig, nullptr, &sample->calculator) != MA_SUCCESS) {
        EST_SetError("Failed to initialize gainer");
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
        EST_SetError("Failed to initialize channel converter");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    sample->data.reserve(4095 * channels);
    *handle = reinterpret_cast<ECHANDLE>(sample);

    return EST_OK;
}

EST_RESULT EST_EncoderLoad(const char *path, est_encoder_callback callback, enum EST_DECODER_FLAGS flags, ECHANDLE *decoder)
{
    if (!path) {
        EST_SetError("Path is not defined");
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

EST_RESULT EST_EncoderLoadMemory(const void *data, int size, est_encoder_callback callback, enum EST_DECODER_FLAGS flags, ECHANDLE *decoder)
{
    if (!data || size == 0) {
        EST_SetError("Path is not defined");
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

EST_RESULT EST_EncoderFree(ECHANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_uninit(&decoder->decoder);
    ma_gainer_uninit(&decoder->gainer, nullptr);
    ma_channel_converter_uninit(&decoder->converter, nullptr);

    delete decoder;
    return EST_OK;
}

EST_RESULT EST_EncoderSeek(ECHANDLE handle, int index)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_seek_to_pcm_frame(&decoder->decoder, index);

    // If using timestretch, we need to process initial buffer
    if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
        decoder->processor->reset();

        int latency = decoder->processor->inputLatency() * 2;
        int readed = 0;

        std::vector<float> convertedData(latency * decoder->channels);

        if (decoder->channels != (int)decoder->decoder.outputChannels) {
            std::vector<float> encoderData(latency * decoder->decoder.outputChannels);

            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&decoder->decoder, &encoderData[0], latency, &ma_readed);

            ma_channel_converter_process_pcm_frames(&decoder->converter, &convertedData[0], &encoderData[0], ma_readed);

            readed = static_cast<int>(ma_readed);
        } else {
            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&decoder->decoder, &convertedData[0], latency, &ma_readed);

            readed = static_cast<int>(ma_readed);
        }

        std::vector<float> outputProcess(convertedData.size());
        decoder->processor->process(convertedData, readed, outputProcess, readed);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderRender(ECHANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_uint64 targetRead = static_cast<ma_uint64>(::floor(decoder->decoder.outputSampleRate * 0.01));

    decoder->data.clear();
    decoder->data.resize(0);
    decoder->processor->reset();
    decoder->processor->presetDefault(
        static_cast<int>(decoder->channels),
        static_cast<float>(decoder->decoder.outputSampleRate));
    decoder->numOfPcmProcessed = 0;

    int                bufferSize = static_cast<int>(::floor(targetRead * decoder->decoder.outputChannels * decoder->rate));
    std::vector<float> buffer(bufferSize);
    std::vector<float> temp(buffer.size());

    /*
     * This is bit tricky, as I want my encoder to support both resampler and timestretch
     *
     * My workflow is:
     * 1. Process resampler
     * 2. -=- timestretch
     * 3. -=- vol/pan
     * 4. -=- user-callback
     * 5. -=- clipping
     */

    EST_EncoderSeek(handle, 0);

    while (true) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        std::fill(temp.begin(), temp.end(), 0.0f);

        ma_uint64 targetThisIteration = targetRead;
        if (decoder->rate != 1.0f) {
            ma_resampler_get_required_input_frame_count(
                &decoder->calculator,
                targetRead,
                &targetThisIteration);
        }

        ma_uint64 readed = 0;
        auto      result = ma_decoder_read_pcm_frames(&decoder->decoder, &buffer[0], targetThisIteration, &readed);
        if (result != MA_SUCCESS || readed == 0) {
            break;
        }

        if (decoder->channels != static_cast<int>(decoder->decoder.outputChannels)) {
            ma_channel_converter_process_pcm_frames(&decoder->converter, &temp[0], &buffer[0], readed);

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::copy(&temp[0], &temp[0] + readed * decoder->channels, &buffer[0]);
        }

        // Effect processing
        {
            if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
                decoder->processor->process(
                    buffer,
                    static_cast<int>(readed),
                    temp,
                    static_cast<int>(targetRead));

                readed = targetRead;

                std::copy(&temp[0], &temp[0] + readed * decoder->channels, &buffer[0]);
            }

            result = ma_gainer_process_pcm_frames(&decoder->gainer, &temp[0], &buffer[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&decoder->panner, &buffer[0], &temp[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }
        }

        int totalReadedThisIteration = static_cast<int>(readed);

        // User-default callback
        if (decoder->callback) {
            decoder->callback(handle, decoder->userData, &buffer[0], totalReadedThisIteration);
        }

        int sizeToCopy = totalReadedThisIteration * decoder->channels;
        std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(decoder->data));

        decoder->numOfPcmProcessed += totalReadedThisIteration;
    }

    // If using timestretch, we need to process the remaining buffer
    if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
        int outputBuffer = decoder->processor->outputLatency();

        int currentCursor = 0;
        while (currentCursor < outputBuffer) {
            ma_uint64 frameToRead = targetRead;

            if (currentCursor + targetRead > outputBuffer) {
                frameToRead = outputBuffer - currentCursor;
            }

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::fill(temp.begin(), temp.end(), 0.0f);

            decoder->processor->process(
                temp,
                0,
                buffer,
                static_cast<int>(frameToRead));

            auto result = ma_gainer_process_pcm_frames(&decoder->gainer, &temp[0], &buffer[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&decoder->panner, &buffer[0], &temp[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            if (decoder->callback) {
                decoder->callback(handle, decoder->userData, &buffer[0], static_cast<int>(frameToRead));
            }

            int sizeToCopy = static_cast<int>(frameToRead) * decoder->channels;
            std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(decoder->data));

            currentCursor += static_cast<int>(frameToRead);
        }
    }

    // Clip the audio data to prevent distortion
    for (int i = 0; i < decoder->data.size(); i++) {
        decoder->data[i] = std::clamp(decoder->data[i], -1.0f, 1.0f);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderSetAttribute(ECHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
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
            EST_SetError("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetAttribute(ECHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
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
            EST_SetError("Attrib is not supported or not found!");
            return EST_ERROR_INVALID_ARGUMENT;
        }
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetData(ECHANDLE handle, void *data, int *size)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->numOfPcmProcessed > 0) {
        float *pData = static_cast<float *>(data);
        int    numOfFloatBytes = decoder->numOfPcmProcessed * decoder->channels;

        std::copy(
            &decoder->data[0],
            &decoder->data[0] + numOfFloatBytes,
            pData);

        *size = decoder->numOfPcmProcessed;
    } else {
        return EST_ERROR_ENCODER_EMPTY;
    }

    return EST_OK;
}

EST_RESULT EST_EncoderFlushData(ECHANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    decoder->data.resize(0);
    decoder->numOfPcmProcessed = 0;

    return EST_OK;
}

EST_RESULT EST_EncoderGetAvailableDataSize(ECHANDLE handle, int *size)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    *size = decoder->numOfPcmProcessed;

    return EST_OK;
}

EST_RESULT EST_EncoderGetSample(ECHANDLE handle, EHANDLE *outSample)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_EncoderRender(handle);

    int maxChannels = decoder->channels;

    auto result = EST_SampleLoadRawPCM(decoder->data.data(), decoder->numOfPcmProcessed, maxChannels, 44100, outSample);
    if (result != EST_OK) {
        return result;
    }

    return EST_OK;
}

EST_RESULT EST_EncoderExportFile(ECHANDLE handle, enum EST_FILE_EXPORT type, char *filePath)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->data.size()) {
        EST_SetError("Decoder not renderer");
        return EST_ERROR_ENCODER_EMPTY;
    }

    if (type == EST_EXPORT_WAV) {
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,
            ma_format_s16,
            decoder->channels,
            decoder->decoder.outputSampleRate);

        ma_encoder encoder;
        ma_result  result = ma_encoder_init_file(filePath, &config, &encoder);
        if (result != MA_SUCCESS) {
            return EST_ERROR_INVALID_ARGUMENT;
        }

        int dataSize = decoder->numOfPcmProcessed * decoder->channels;

        // Convert data to signed int16
        std::vector<int16_t> data(dataSize);
        for (int i = 0; i < dataSize; i++) {
            float f = decoder->data[i];
            f = std::clamp(f * 32768.0f, -32768.0f, 32767.0f);

            int16_t val = static_cast<int16_t>(f);
            data[i] = val;
        }

        ma_uint64 written = 0;
        result = ma_encoder_write_pcm_frames(&encoder, &data[0], decoder->numOfPcmProcessed, &written);
        if (result != MA_SUCCESS) {
            return EST_ERROR_ENCODER_INVALID_WRITE;
        }

        ma_encoder_uninit(&encoder);
    } else {
        EST_SetError("Invalid export type or unsupported");
        return EST_ERROR_ENCODER_UNSUPPORTED;
    }

    return EST_OK;
}