#include "Internal.h"

namespace {
    constexpr int kMaxChannels = 2;
} // namespace

static ma_uint32 data_mix_pcm(EST_Device *device, EST_Channel *channel, float *pOutput, ma_uint32 frameCount)
{
    int       channels = device->channels;
    ma_result result = MA_SUCCESS;

    auto &temp = device->processingData;
    auto &temp2 = device->temporaryData;

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

        if (channel->attributes.rate != 1.0f) {
            ma_resampler_get_required_input_frame_count(
                &channel->pitch->resampler,
                framesToReadThisIteration,
                &framesToReadThisIteration);
        }

        framesReadThisIteration = ma_audio_buffer_read_pcm_frames(&channel->buffer, &temp[0], framesToReadThisIteration, MA_FALSE);
        if (framesReadThisIteration == 0) {
            break;
        }

        if (channel->channels != device->channels) {
            ma_channel_converter_process_pcm_frames(&channel->converter, &temp2[0], &temp[0], framesReadThisIteration);

            std::fill(temp.begin(), temp.begin() + byteSize, 0.0f);
            std::copy(&temp2[0], &temp2[0] + framesReadThisIteration * device->channels, &temp[0]);
        }

        if (channel->attributes.rate != 1.0f) {
            result = ma_resampler_process_pcm_frames(&channel->pitch->resampler, &temp[0], &framesReadThisIteration, &temp2[0], &expectedToReadThisIteration);

            if (result != MA_SUCCESS) {
                break;
            }

            framesReadThisIteration = expectedToReadThisIteration;

            if (!channel->pitch->isPitched) {
                channel->pitch->processor->process(
                    temp2,
                    static_cast<int>(framesReadThisIteration),
                    temp,
                    static_cast<int>(framesReadThisIteration));
            } else {
                std::copy(temp2.begin(), temp2.begin() + framesReadThisIteration * channels, temp.begin());
            }
        }

        result = ma_panner_process_pcm_frames(&channel->panner, &temp2[0], &temp[0], framesReadThisIteration);
        if (result != MA_SUCCESS) {
            break;
        }

        result = ma_gainer_process_pcm_frames(&channel->gainer, &temp[0], &temp2[0], framesReadThisIteration);
        if (result != MA_SUCCESS) {
            break;
        }

        /* Mix the frames together. */
        for (iSample = 0; iSample < framesReadThisIteration * channels; ++iSample) {
            pOutput[totalFramesRead * channels + iSample] += temp[iSample]; // std::clamp(temp[iSample], -1.0f, 1.0f);
        }

        totalFramesRead += (ma_uint32)framesReadThisIteration;

        if (framesReadThisIteration < (ma_uint32)framesToReadThisIteration) {
            break; /* Reached EOF. */
        }
    }

    if (channel->callbacks.size()) {
        for (auto &it : device->callbacks) {
            it.callback(channel, it.userdata, pOutput, totalFramesRead);
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

static void data_callback(ma_device *pObject, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    if (!pObject->pUserData) {
        return;
    }

    EST_Device *device = reinterpret_cast<EST_Device *>(pObject->pUserData);

    float *pOutputFloat = reinterpret_cast<float *>(pOutput);

    for (auto channel : device->channel_arrays) {
        if (channel->isPlaying) {
            ma_uint32 pcmReaded = data_mix_pcm(device, channel.get(), pOutputFloat, frameCount);

            if (pcmReaded < frameCount) {
                if (channel->attributes.looping) {
                    ma_audio_buffer_seek_to_pcm_frame(&channel->buffer, 0);
                } else {
                    channel->isAtEnd = true;
                    channel->isPlaying = false;
                }
            }
        }
    }

    // erase_if_map(
    //     device->channel_arrays,
    //     [&](std::pair<EST_AUDIO_HANDLE, std::shared_ptr<EST_Channel>> sample) {
    //         return sample.second->isRemoved;
    //     });

    device->channel_arrays.erase(
        std::remove_if(
            device->channel_arrays.begin(),
            device->channel_arrays.end(),
            [](std::shared_ptr<EST_Channel> &channel) {
                return channel->isRemoved;
            }),
        device->channel_arrays.end());

    if (device->callbacks.size()) {
        for (auto &it : device->callbacks) {
            it.callback(nullptr, it.userdata, pOutputFloat, frameCount);
        }
    }

    int frameCountInBytes = frameCount * device->channels;
    for (int i = 0; i < frameCountInBytes; i++) {
        pOutputFloat[i] = std::clamp(pOutputFloat[i], -1.0f, 1.0f);
    }

    (void)pObject;
    (void)pInput;
}

struct EST_Device *EST_DeviceInit(int sampleRate, enum EST_DEVICE_FLAGS flags)
{
    EST_Device *device = nullptr;

    try {
        device = new EST_Device;
    } catch (std::bad_alloc &alloc) {
        EST_ErrorSetMessage(alloc.what());
        return nullptr;
    }

    int channels = 2;
    if (FLAG_EXIST(flags, EST_DEVICE_MONO)) {
        channels = 1;
    }

    if (FLAG_EXIST(flags, EST_DEVICE_FORMAT_S16)) {
        EST_ErrorSetMessage("'EST_DEVICE_FORMAT_S16' is not yet supported!");

        delete device;
        return nullptr;
    }

    device->channels = channels;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = channels;
    config.sampleRate = sampleRate;
    config.dataCallback = data_callback;
    config.periodSizeInMilliseconds = 0;
    config.pUserData = reinterpret_cast<void *>(device);

    auto result = ma_device_init(NULL, &config, &device->device);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to initialize audio device");

        delete device;
        return nullptr;
    }

    if (ma_device_start(&device->device) != MA_SUCCESS) {
        EST_ErrorSetMessage("Failed to start audio device");
        ma_device_uninit(&device->device);

        delete device;
        return nullptr;
    }

    device->temporaryData.resize(4095 * kMaxChannels);
    device->processingData.resize(4095 * kMaxChannels);

    device->mutex = std::make_shared<std::mutex>();
    return device;
}

EST_RESULT EST_GetInfo(EST_Device *device, est_device_info *info)
{
    if (!device) {
        EST_ErrorSetMessage("No context");
        return EST_ERROR_INVALID_STATE;
    }

    info->channels = device->channels;
    info->deviceIndex = -1;
    info->sampleRate = device->device.sampleRate;
    info->flags = EST_DEVICE_UNKNOWN;

    return EST_OK;
}

EST_RESULT EST_DeviceFree(EST_Device *device)
{
    if (!device) {
        EST_ErrorSetMessage("No context");
        return EST_ERROR_INVALID_STATE;
    }

    for (auto &channel : device->channel_arrays) {
        channel->isRemoved = true;
    }

    // wait for all channels to be removed in audio thread
    while (device->channel_arrays.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    ma_device_uninit(&device->device);

    delete device;
    return EST_OK;
}