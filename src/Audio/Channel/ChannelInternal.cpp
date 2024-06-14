#include "ChannelInternal.h"

struct EST_Channel *InternalInit(EST_Device *device, float *data_pointer, int channels, int pcmSize, int sampleRate)
{
    auto channel = std::make_shared<EST_Channel>();
    if (!channel) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to allocate memory");
        return nullptr;
    }

    if (data_pointer == nullptr) {
        EST_ErrorSetMessage("EST_ChannelLoad: data_pointer is empty");
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

    channel->channels = channels;

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
        pitch = std::shared_ptr<EST_ChannelResampler>(new EST_ChannelResampler, EST_ResamplerDestructor{});
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

    std::lock_guard<std::mutex> lock(*device->mutex.get());
    device->channel_arrays.push_back(channel);

    return channel.get();
}