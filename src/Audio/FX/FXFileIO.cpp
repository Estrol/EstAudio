#include "../Internal.h"
#include "../../Utils/IO.h"
#include "../../Encoder/EncoderInternal.h"

#include "../../Utils/IO.h"
#include "../Channel/ChannelInternal.h"

EST_FX *FXInternalLoad(std::vector<float> &data, int channels, int sampleRate, int pcmSize)
{
    EST_FX *fx = new EST_FX();
    if (!fx) {
        EST_ErrorSetMessage("EST_FXLoad: Failed to allocate memory for EST_FX");
        return nullptr;
    }

    fx->data = data;
    fx->channels = channels;
    fx->pcmSize = pcmSize;
    fx->sampleRate = sampleRate;
    fx->processor = std::make_shared<SignalsmithStretch>();
    fx->processor->presetDefault(channels, static_cast<float>(sampleRate));

    ma_resampler_config config = ma_resampler_config_init(
        ma_format_f32,
        channels,
        sampleRate,
        sampleRate,
        ma_resample_algorithm_linear);

    ma_result result = ma_resampler_init(&config, nullptr, &fx->resampler);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoad: Failed to initialize resampler");
        return nullptr;
    }

    return fx;
}

EST_FX *EST_FXLoad(const char *path)
{
    if (!std::filesystem::exists(path)) {
        EST_ErrorSetMessage("EST_FXLoad: File does not exist");
        return nullptr;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);

    ma_decoder decoder;
    ma_result  result = ma_decoder_init_file(path, &config, &decoder);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoad: Failed to load audio file");
        return nullptr;
    }

    ma_uint64 framesAvailable = 0;
    result = ma_decoder_get_available_frames(&decoder, &framesAvailable);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoad: Failed to get available frames");
        return nullptr;
    }

    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);

    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoad: Failed to get data format");
        return nullptr;
    }

    std::vector<float> pcmData;
    std::vector<float> buffer(4095 * channels);
    ma_uint64          pcmToRead = 4095;
    ma_uint64          pcmSize = 0;
    while (true) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS || framesRead == 0) {
            if (result != MA_AT_END) {
                EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to read pcm frames");
                return nullptr;
            }

            if (framesRead <= 0) {
                break; // let it break here
            }
        }

        std::copy(buffer.begin(), buffer.begin() + framesRead * channels, std::back_inserter(pcmData));
        pcmSize += framesRead;
    }

    ma_decoder_uninit(&decoder);

    return FXInternalLoad(pcmData, (int)channels, (int)sampleRate, (int)pcmSize);
}

EST_FX *EST_FXLoadFromMemory(const void *data, size_t size)
{
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);

    ma_decoder decoder;
    ma_result  result = ma_decoder_init_memory(data, size, &config, &decoder);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoadFromMemory: Failed to load audio file");
        return nullptr;
    }

    ma_uint64 framesAvailable = 0;
    result = ma_decoder_get_available_frames(&decoder, &framesAvailable);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoadFromMemory: Failed to get available frames");
        return nullptr;
    }

    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);

    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_FXLoadFromMemory: Failed to get data format");
        return nullptr;
    }

    std::vector<float> pcmData;
    std::vector<float> buffer(4095 * channels);
    ma_uint64          pcmToRead = 4095;
    ma_uint64          pcmSize = 0;
    while (true) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS || framesRead == 0) {
            if (result != MA_AT_END) {
                EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to read pcm frames");
                return nullptr;
            }

            if (framesRead <= 0) {
                break; // let it break here
            }
        }

        std::copy(buffer.begin(), buffer.begin() + framesRead * channels, std::back_inserter(pcmData));
        pcmSize += framesRead;
    }

    ma_decoder_uninit(&decoder);

    return FXInternalLoad(pcmData, (int)channels, (int)sampleRate, (int)pcmSize);
}

void EST_FXFree(EST_FX *fx)
{
    if (!fx) {
        return;
    }

    ma_resampler_uninit(&fx->resampler, nullptr);

    delete fx;
}

EST_Sample *EST_FXCreateSample(EST_FX *fx)
{
    if (!fx) {
        EST_ErrorSetMessage("EST_FXCreateSample: Invalid EST_FX");
        return nullptr;
    }

    EST_Sample *sample = new EST_Sample();
    if (!sample) {
        EST_ErrorSetMessage("EST_FXCreateSample: Failed to allocate memory for EST_Sample");
        return nullptr;
    }

    sample->channels = fx->channels;
    sample->sampleRate = fx->sampleRate;
    sample->data = fx->data;
    sample->pcmSize = fx->pcmSize;
    sample->fx = fx;

    return sample;
}

EST_Channel *EST_FXCreateChannel(EST_FX *fx, EST_Device *device)
{
    if (!fx) {
        EST_ErrorSetMessage("EST_FXCreateChannel: Invalid EST_FX");
        return nullptr;
    }

    if (!device) {
        EST_ErrorSetMessage("EST_FXCreateChannel: Invalid EST_Device");
        return nullptr;
    }

    auto channel = ChannelInternalInitMemory(
        device,
        &fx->data[0],
        fx->channels,
        fx->pcmSize,
        fx->sampleRate,
        fx);

    if (!channel) {
        EST_ErrorSetMessage("EST_FXCreateChannel: Failed to create channel");
        return nullptr;
    }

    return channel;
}