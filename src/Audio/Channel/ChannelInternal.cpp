#include "ChannelInternal.h"
#include <kissfft/kiss_fft.h>
#include <kissfft/kiss_fftr.h>

void ChannelInternalFree(EST_Channel *channel)
{
    if (channel == nullptr) {
        return;
    }

    ma_audio_buffer_uninit(&channel->buffer);
    ma_gainer_uninit(&channel->gainer, nullptr);
    ma_channel_converter_uninit(&channel->converter, nullptr);
    ma_resampler_uninit(&channel->resampler, nullptr);
}

struct EST_Channel *Init(EST_Device *device, std::shared_ptr<EST_Channel> channel, float *data_pointer, int channels, int pcmSize, int sampleRate, EST_FX *fx)
{
    ma_audio_buffer_config config = ma_audio_buffer_config_init(
        ma_format_f32,
        channels,
        pcmSize,
        data_pointer,
        nullptr);

    ma_result result = ma_audio_buffer_init(&config, &channel->buffer);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to initialize audio buffer");
        return nullptr;
    }

    ma_panner_config pannerConfig = ma_panner_config_init(ma_format_f32, channels);
    if (ma_panner_init(&pannerConfig, &channel->panner) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize panner");
        return nullptr;
    }

    ma_gainer_config gainerConfig = ma_gainer_config_init(channels, 0);
    if (ma_gainer_init(&gainerConfig, nullptr, &channel->gainer) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize gainer");
        return nullptr;
    }

    ma_resampler_config resamplerConfig = ma_resampler_config_init(
        ma_format_f32,
        channels,
        device->device.sampleRate,
        device->device.sampleRate,
        ma_resample_algorithm_linear);

    if (ma_resampler_init(&resamplerConfig, nullptr, &channel->resampler) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize gainer");
        return nullptr;
    }

    ma_channel_converter_config chConfig = ma_channel_converter_config_init(
        ma_format_f32,                // Sample format
        channels,                     // Input channels
        NULL,                         // Input channel map
        device->channels,             // Output channels
        NULL,                         // Output channel map
        ma_channel_mix_mode_default); // The mixing algorithm to use when combining channels.

    if (ma_channel_converter_init(&chConfig, nullptr, &channel->converter)) {
        EST_ErrorSetMessage("Failed to initialize channel converter");
        return nullptr;
    }

    channel->isInit = true;
    channel->channels = channels;
    channel->sampleRate = sampleRate;
    channel->buffer.ref.sampleRate = sampleRate;

    if (fx != nullptr) {
        auto fxInstance = new EST_FX_Instance();

        fxInstance->attributes = fx->attributes;
        fxInstance->processor = std::make_shared<SignalsmithStretch>();
        fxInstance->processor->presetDefault(channels, static_cast<float>(sampleRate));

        ma_resampler_config resamplerConfig2 = ma_resampler_config_init(
            ma_format_f32,
            channels,
            sampleRate,
            sampleRate,
            ma_resample_algorithm_linear);

        result = ma_resampler_init(&resamplerConfig2, nullptr, &fxInstance->resampler);
        if (result != MA_SUCCESS) {
            EST_ErrorSetMessage("Failed to initialize resampler");
            return nullptr;
        }

        channel->fx = fxInstance;
    }

    // Seek to the beginning of the channel, to refresh FX buffer if necessary
    EST_ChannelSeekPosition(channel.get(), EST_CHANNEL_POSITION_SAMPLES, 0);

    std::lock_guard<std::mutex> lock(*device->mutex.get());
    device->channel_arrays.push_back(channel);

    return channel.get();
}

struct EST_Channel *ChannelInternalInit(EST_Device *device, std::string hash, int channels, int pcmSize, int sampleRate, EST_FX *fx)
{
    auto channel = std::make_shared<EST_Channel>();
    if (!channel) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to allocate memory");
        return nullptr;
    }

    if (device->memory.find(hash) == device->memory.end()) {
        EST_ErrorSetMessage("EST_ChannelLoad: invalid data pointer hash");
        return nullptr;
    }

    if (pcmSize == 0) {
        EST_ErrorSetMessage("EST_ChannelLoad: pcmSize is 0");
        return nullptr;
    }

    if (channels == 0) {
        EST_ErrorSetMessage("EST_ChannelLoad: channels is 0");
        return nullptr;
    }

    channel->memoryHash = hash;
    float *data_pointer = &device->memory[hash].data[0];

    return Init(device, channel, data_pointer, channels, pcmSize, sampleRate, fx);
}

struct EST_Channel *ChannelInternalInitMemory(EST_Device *device, float *data_pointer, int channels, int pcmSize, int sampleRate, EST_FX *fx)
{
    auto channel = std::make_shared<EST_Channel>();
    if (!channel) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to allocate memory");
        return nullptr;
    }

    if (data_pointer == nullptr) {
        EST_ErrorSetMessage("EST_ChannelLoad: invalid data pointer hash");
        return nullptr;
    }

    if (pcmSize == 0) {
        EST_ErrorSetMessage("EST_ChannelLoad: pcmSize is 0");
        return nullptr;
    }

    if (channels == 0) {
        EST_ErrorSetMessage("EST_ChannelLoad: channels is 0");
        return nullptr;
    }

    return Init(device, channel, data_pointer, channels, pcmSize, sampleRate, fx);
}

int ChannelInternalFFTGetData(EST_Channel *channel, float *data, int size, bool individual)
{
    if (channel == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is nullptr");
        return -1;
    }

    if (data == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: data is nullptr");
        return -1;
    }

    if (size <= 0 || (size & (size - 1)) != 0) {
        EST_ErrorSetMessage("ChannelInternalGetData: size is not positive and power of 2");
        return -1;
    }

    if (channel->buffer.ref.channels != 2) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is not stereo");
        return -1;
    }

    if (size % 2 != 0) {
        EST_ErrorSetMessage("ChannelInternalGetData: size is not even");
        return -1;
    }

    int fft_size = size / 2;

    kiss_fft_cpx *out = new kiss_fft_cpx[fft_size + 1];
    kiss_fftr_cfg cfg = kiss_fftr_alloc(fft_size * 2, 0, NULL, NULL);

    ma_uint64 currentFramePos = 0;
    ma_uint64 maxFramePos = 0;

    ma_audio_buffer_get_cursor_in_pcm_frames(&channel->buffer, &currentFramePos);
    ma_audio_buffer_get_available_frames(&channel->buffer, &maxFramePos);

    ma_uint64 index_pos = currentFramePos * channel->channels;
    ma_uint64 to_read = size;

    if (index_pos >= maxFramePos * channel->channels) {
        index_pos = maxFramePos * channel->channels - 1;
        to_read = 0;
    } else if (index_pos + to_read > maxFramePos * channel->channels) {
        to_read = maxFramePos * channel->channels - index_pos;
    }

    const float *bufData = reinterpret_cast<const float *>(channel->buffer.ref.pData);
    float       *bufData2 = new float[to_read];
    memcpy(bufData2, bufData + index_pos, to_read * sizeof(float));

    kiss_fftr(cfg, bufData2, out);

    float scaling = 1.0f / (float)fft_size;
    if (individual) {
        for (int i = 0; i < fft_size * 2; i += 2) {
            data[i] = 2.0f * sqrt(out[i].r * out[i].r) * scaling;
            data[i + 1] = 2.0f * sqrt(out[i + 1].i * out[i + 1].i) * scaling;
        }
    } else {
        for (int i = 0; i < fft_size; i++) {
            data[i] = 2.0f * sqrt(out[i].r * out[i].r + out[i + 1].r * out[i + 1].r) * scaling;
        }
    }

    delete[] bufData2;
    delete[] out;
    kiss_fftr_free(cfg);

    return (int)fft_size;
}

int ChannelInternalGetData(EST_Channel *channel, float *data, int size)
{
    if (channel == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is nullptr");
        return -1;
    }

    if (data == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: data is nullptr");
        return -1;
    }

    if (size <= 0) {
        EST_ErrorSetMessage("ChannelInternalGetData: size is not positive");
        return -1;
    }

    ma_uint64 currentFramePos = 0;
    ma_uint64 maxFramePos = 0;
    ma_audio_buffer_get_cursor_in_pcm_frames(&channel->buffer, &currentFramePos);
    ma_audio_buffer_get_available_frames(&channel->buffer, &maxFramePos);

    ma_uint64 index_pos = currentFramePos * channel->channels;
    ma_uint64 to_read = size;

    if (index_pos >= maxFramePos * channel->channels) {
        index_pos = maxFramePos * channel->channels - 1;
        to_read = 0;
    } else if (index_pos + to_read > maxFramePos * channel->channels) {
        to_read = maxFramePos * channel->channels - index_pos;
    }

    const float *bufData = reinterpret_cast<const float *>(channel->buffer.ref.pData);
    memcpy(data, bufData + index_pos, to_read * sizeof(float));

    return (int)to_read;
}

EST_RESULT ChannelInternalSetPositionMS(EST_Channel *channel, float ms)
{
    if (channel == nullptr) {
        EST_ErrorSetMessage("ChannelInternalSetPositionMS: channel is nullptr");
        return EST_ERROR;
    }

    // calculate frame position from ms
    ma_uint64 framePos = static_cast<ma_uint64>(ms * channel->sampleRate / 1000.0f);
    ma_audio_buffer_seek_to_pcm_frame(&channel->buffer, framePos);

    return EST_OK;
}