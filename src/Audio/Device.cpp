/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "Internal.h"
#include "Channel/ChannelInternal.h"

namespace {
    constexpr int kMaxChannels = 2;
}

static ma_uint64 read_pcm_data(EST_Channel *channel, float *output, ma_uint64 frameCount)
{
    std::array<float, 4096 * kMaxChannels> temp = {}, temp2 = {};

    if (channel->fx != nullptr) {
        auto fx = channel->fx;
        if (fx->lock) {
            return frameCount;
        }

        ma_uint64 targetToReadThisIteration = frameCount;
        ma_uint64 targetThisIteration = frameCount;

        if (fx->attributes.tempo != 1.0f) {
            ma_resampler_get_required_input_frame_count(
                &fx->resampler,
                frameCount,
                &targetToReadThisIteration);
        }

        ma_uint64 availableFrames = 0;
        ma_audio_buffer_get_available_frames(&channel->buffer, &availableFrames);
        if (availableFrames > 0) {
            targetToReadThisIteration = ma_audio_buffer_read_pcm_frames(
                &channel->buffer,
                &temp[0],
                targetToReadThisIteration,
                MA_FALSE);

            if (targetToReadThisIteration >= availableFrames) {
                fx->framesAvailable += fx->processor->outputLatency();
            } else {
                fx->framesAvailable += static_cast<int>(targetThisIteration);
            }
        }

        if (fx->framesAvailable > 0) {
            fx->processor->process(
                &temp[0],
                static_cast<int>(targetToReadThisIteration),
                &temp2[0],
                static_cast<int>(targetThisIteration));

            fx->framesAvailable -= static_cast<int>(targetThisIteration);

            if (fx->framesAvailable < 0) {
                targetThisIteration += fx->framesAvailable;
                fx->framesAvailable = 0;
            }
        } else {
            targetThisIteration = 0;
        }

        std::copy(&temp2[0], &temp2[0] + targetThisIteration * channel->channels, output);

        return targetThisIteration;
    } else {
        ma_uint64 readed = ma_audio_buffer_read_pcm_frames(&channel->buffer, output, frameCount, MA_FALSE);
        if (readed == 0) {
            return 0;
        }

        return readed;
    }
}

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

        if ((ma_uint32)channel->attributes.samplerate != channel->buffer.ref.sampleRate) {
            result = ma_resampler_get_required_input_frame_count(
                &channel->resampler,
                framesToReadThisIteration,
                &framesToReadThisIteration);

            if (result != MA_SUCCESS) {
                break;
            }
        }

        framesReadThisIteration = read_pcm_data(
            channel,
            &temp[0],
            framesToReadThisIteration);

        if (channel->channels != device->channels) {
            ma_channel_converter_process_pcm_frames(
                &channel->converter,
                &temp2[0],
                &temp[0],
                framesReadThisIteration);

            std::fill(temp.begin(), temp.begin() + byteSize, 0.0f);
            std::copy(&temp2[0], &temp2[0] + framesReadThisIteration * device->channels, &temp[0]);
        }

        if ((ma_uint32)channel->attributes.samplerate != channel->buffer.ref.sampleRate) {
            result = ma_resampler_process_pcm_frames(
                &channel->resampler,
                &temp[0],
                &framesReadThisIteration,
                &temp2[0],
                &expectedToReadThisIteration);

            if (result != MA_SUCCESS) {
                break;
            }

            framesReadThisIteration = expectedToReadThisIteration;

            std::copy(temp2.begin(), temp2.begin() + framesReadThisIteration * channels, temp.begin());
        }

        result = ma_panner_process_pcm_frames(
            &channel->panner,
            &temp2[0],
            &temp[0],
            framesReadThisIteration);

        if (result != MA_SUCCESS) {
            break;
        }

        result = ma_gainer_process_pcm_frames(
            &channel->gainer,
            &temp[0],
            &temp2[0],
            framesReadThisIteration);

        if (result != MA_SUCCESS) {
            break;
        }

        /* Mix the frames together. */
        for (iSample = 0; iSample < framesReadThisIteration * channels; ++iSample) {
            pOutput[totalFramesRead * channels + iSample] += temp[iSample];
        }

        totalFramesRead += static_cast<ma_uint32>(framesReadThisIteration);

        if (framesReadThisIteration < static_cast<ma_uint32>(framesToReadThisIteration)) {
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
    auto beg = items.begin();
    auto end = items.end();

    for (; beg != end;) {
        if (predicate(*beg)) {
            beg = items.erase(beg);
        } else {
            ++beg;
        }
    }
}

template <typename ContainerT, typename PredicateT>
void erase_if(const ContainerT &items, const PredicateT &predicate)
{
    auto it = std::remove_if(items.begin(), items.end(), predicate);
    items.erase(it, items.end());
}

static void data_callback(ma_device *pObject, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    if (!pObject->pUserData) {
        return;
    }

    EST_Device *device = reinterpret_cast<EST_Device *>(pObject->pUserData);
    auto        now = std::chrono::high_resolution_clock::now();
    auto        duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - device->time).count();
    float       delta = static_cast<float>(duration) / 1000.0f;

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

    for (auto channel : device->guest_channel_arrays) {
        if (channel->isPlaying) {
            ma_uint32 pcmReaded = data_mix_pcm(device, channel, pOutputFloat, frameCount);

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

    std::function on_delete_callback = [&device](std::shared_ptr<EST_Channel> &channel) {
        if (device->memory.find(channel->memoryHash) != device->memory.end()) {
            auto &item = device->memory[channel->memoryHash];
            item.useCount--;
        }

        ChannelInternalFree(channel.get());
    };

    std::function erase_callback = [&on_delete_callback](std::shared_ptr<EST_Channel> &channel) {
        if (channel->isRemoved) {
            on_delete_callback(channel);
        }

        return channel->isRemoved;
    };

    std::function erase_check = [](std::shared_ptr<EST_Channel> &channel) {
        return channel->isRemoved;
    };

    bool anyRemoved = std::any_of(device->channel_arrays.begin(), device->channel_arrays.end(), erase_check);
    if (anyRemoved) {
        std::lock_guard<std::mutex> lock(*device->mutex);

        erase_if(device->channel_arrays, erase_callback);
    }

    std::function on_memory_callback = [&device, &delta](std::pair<const std::string, EST_MemoryItem> &memory) {
        if (memory.second.useTimeout >= EST_MEMORY_TIMEOUT) {
            return true;
        }

        return false;
    };

    for (auto &[key, value] : device->memory) {
        if (value.useCount <= 0) {
            value.useTimeout += delta;
        } else {
            value.useTimeout = 0.0f;
        }
    }

    std::function memory_callback_check = [](std::pair<const std::string, EST_MemoryItem> &memory) {
        return memory.second.useTimeout >= EST_MEMORY_TIMEOUT;
    };

    bool anyMemoryRemoved = std::any_of(device->memory.begin(), device->memory.end(), memory_callback_check);
    if (anyMemoryRemoved) {
        std::lock_guard<std::mutex> lock(*device->mutex);

        erase_if_map(device->memory, on_memory_callback);
    }

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

    device->time = now;
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

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(device);
    if (unknown->type != EST_UNKNOWN_DEVICE) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
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

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(device);
    if (unknown->type != EST_UNKNOWN_DEVICE) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
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

EST_DataCallback *EST_DeviceAddCallback(EST_Device *device, EST_DATA_CALLBACK callback, void *userData)
{
    if (!device) {
        return nullptr;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(device);
    if (unknown->type != EST_UNKNOWN_DEVICE) {
        EST_ErrorSetMessage("Invalid handle");
        return nullptr;
    }

    EST_DataCallback callbackData;
    callbackData.callback = callback;
    callbackData.userdata = userData;

    device->callbacks.push_back(callbackData);

    return &device->callbacks[device->callbacks.size() - 1];
}

EST_RESULT EST_DeviceRemoveCallback(EST_Device *device, EST_DataCallback *callback)
{
    if (!device) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = reinterpret_cast<EST_Unknown *>(device);
    if (unknown->type != EST_UNKNOWN_DEVICE) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    unknown = reinterpret_cast<EST_Unknown *>(callback);
    if (unknown->type != EST_UNKNOWN_DATA_CALLBACK) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    for (int i = 0; i < device->callbacks.size(); i++) {
        if (&device->callbacks[i] == callback) {
            device->callbacks.erase(device->callbacks.begin() + i);
            return EST_OK;
        }
    }

    return EST_ERROR_INVALID_ARGUMENT;
}