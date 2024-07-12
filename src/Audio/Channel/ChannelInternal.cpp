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

    if (channel->pitch) {
        channel->pitch->processor->reset();
        ma_resampler_uninit(&channel->pitch->resampler, nullptr);
    }
}

struct EST_Channel *Init(EST_Device *device, std::shared_ptr<EST_Channel> channel, float *data_pointer, int channels, int pcmSize, int sampleRate)
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

    std::shared_ptr<EST_ChannelResampler> pitch;

    try {
        pitch = std::make_shared<EST_ChannelResampler>();
    } catch (std::bad_alloc &alloc) {
        EST_ErrorSetMessage(alloc.what());
        return nullptr;
    }

    pitch->processor = std::make_shared<SignalsmithStretch>();
    pitch->processor->presetCheaper(channels, static_cast<float>(sampleRate));

    if (ma_resampler_init(&resamplerConfig, nullptr, &pitch->resampler) != MA_SUCCESS) {
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

    pitch->isInit = true;
    channel->isInit = true;
    channel->channels = channels;
    channel->pitch = pitch;
    channel->sampleRate = sampleRate;
    channel->buffer.ref.sampleRate = sampleRate;

    std::lock_guard<std::mutex> lock(*device->mutex.get());
    device->channel_arrays.push_back(channel);

    return channel.get();
}

struct EST_Channel *ChannelInternalInit(EST_Device *device, std::string hash, int channels, int pcmSize, int sampleRate)
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

    return Init(device, channel, data_pointer, channels, pcmSize, sampleRate);
}

struct EST_Channel *ChannelInternalInitMemory(EST_Device *device, float *data_pointer, int channels, int pcmSize, int sampleRate)
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

    return Init(device, channel, data_pointer, channels, pcmSize, sampleRate);
}

EST_RESULT ChannelInternalFFTGetData(EST_Channel *channel, float *data, int size, bool individual)
{
    if (channel == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is nullptr");
        return EST_ERROR;
    }

    if (data == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: data is nullptr");
        return EST_ERROR;
    }

    if (size <= 0 || (size & (size - 1)) != 0) {
        EST_ErrorSetMessage("ChannelInternalGetData: size is not positive and power of 2");
        return EST_ERROR;
    }

    if (channel->buffer.ref.channels != 2) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is not stereo");
        return EST_ERROR;
    }

    std::vector<float> buffer(size);

    ma_uint64 currentFramePos = 0;
    ma_uint64 maxFramePos = 0;
    ma_audio_buffer_get_cursor_in_pcm_frames(&channel->buffer, &currentFramePos);
    ma_audio_buffer_get_available_frames(&channel->buffer, &maxFramePos);

    ma_uint64 index_pos = currentFramePos * 2;
    ma_uint64 to_read = size;

    if (index_pos >= maxFramePos * 2) {
        index_pos = maxFramePos * 2 - 1;
        to_read = 0;
    } else if (index_pos + to_read > maxFramePos * 2) {
        to_read = maxFramePos * 2 - index_pos;
    }

    const float* bufData = reinterpret_cast<const float*>(channel->buffer.ref.pData);
    memcpy(buffer.data(), bufData + index_pos, to_read * sizeof(float));

    int nfft = size / 2;

    std::vector<float> leftChannel(nfft);
    std::vector<float> rightChannel(nfft);

    for (int i = 0; i < nfft; i++) {
        leftChannel[i] = buffer[i * 2]; // Adjust for stereo interleaving
        rightChannel[i] = buffer[i * 2 + 1]; // Adjust for stereo interleaving
    }

    kiss_fftr_cfg cfg = kiss_fftr_alloc(nfft, 0, NULL, NULL);
    if (cfg == NULL) {
        EST_ErrorSetMessage("ChannelInternalFFTGetData: failed to allocate kiss_fftr_cfg");
        return EST_ERROR;
    }

    std::vector<kiss_fft_cpx> leftChannelFFT(nfft/2 + 1); // Adjust for FFT output size
    std::vector<kiss_fft_cpx> rightChannelFFT(nfft/2 + 1); // Adjust for FFT output size

    kiss_fftr(cfg, leftChannel.data(), leftChannelFFT.data());
    kiss_fftr(cfg, rightChannel.data(), rightChannelFFT.data());

    for (int i = 0; i < nfft/2; i++) {
        double leftMagnitude = sqrt(static_cast<double>(leftChannelFFT[i].r) * leftChannelFFT[i].r + static_cast<double>(leftChannelFFT[i].i) * leftChannelFFT[i].i);
        double rightMagnitude = sqrt(static_cast<double>(rightChannelFFT[i].r) * rightChannelFFT[i].r + static_cast<double>(rightChannelFFT[i].i) * rightChannelFFT[i].i);
        if (individual) {
            data[i] = static_cast<float>(leftMagnitude); // Cast to float if necessary
            data[i + nfft/2] = static_cast<float>(rightMagnitude); // Cast to float if necessary
        } else {
            // Averaging the magnitudes for a mono signal, cast to float if necessary
            data[i] = static_cast<float>((leftMagnitude + rightMagnitude) / 2.0);
        }
    }

    kiss_fftr_free(cfg);

    return EST_OK;
}

EST_RESULT ChannelInternalGetData(EST_Channel *channel, float *data, int size)
{
    if (channel == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: channel is nullptr");
        return EST_ERROR;
    }

    if (data == nullptr) {
        EST_ErrorSetMessage("ChannelInternalGetData: data is nullptr");
        return EST_ERROR;
    }

    if (size <= 0) {
        EST_ErrorSetMessage("ChannelInternalGetData: size is not positive");
        return EST_ERROR;
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

    const float* bufData = reinterpret_cast<const float*>(channel->buffer.ref.pData);
    memcpy(data, bufData + index_pos, to_read * sizeof(float));

    return EST_OK;
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