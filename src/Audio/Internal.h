#ifndef __AUDIO_INTERNAL_H_
#define __AUDIO_INTERNAL_H_

#include <EstAudio.h>

#include "../third-party/miniaudio/miniaudio_decoders.h"
#include "../third-party/signalsmith-stretch/signalsmith-stretch.h"

using namespace signalsmith::stretch;
#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define FLAG_EXIST(flags, flag) ((flags & flag) == flag)

struct EST_ChannelDataCallback
{
    est_channel_data_callback callback;
    void                     *userdata;
};

struct EST_ChannelResampler
{
    bool isInit = false;
    bool isPitched = true;
    bool isAtEnd = false;

    ma_resampler                        resampler = {};
    std::shared_ptr<SignalsmithStretch> processor = {};
};

struct EST_Attribute
{
    float volume = 1.0f;
    float rate = 1.0f;
    float pitch = 1.0f;
    float pan = 0.0f;
    bool  looping = false;
};

#define EST_SAMPLE_MAGIC "ESTS"

struct EST_Sample
{
    const char signature[5] = EST_SAMPLE_MAGIC;

    int channels = 0;
    int sampleRate = 0;
    int pcmSize = 0;

    std::vector<float> data = {};
};

struct EST_ResamplerDestructor
{
    inline void operator()(EST_ChannelResampler *sample) const
    {
        if (!sample->isInit) {
            return;
        }

        ma_resampler_uninit(&sample->resampler, nullptr);
        sample->processor->reset();
    }
};

#define EST_CHANNEL_MAGIC "ESTC"

struct EST_Channel
{
    const char magic[5] = EST_CHANNEL_MAGIC;

    int channels = 0;

    ma_audio_buffer      buffer = {};
    ma_panner            panner = {};
    ma_gainer            gainer = {};
    ma_channel_converter converter = {};
    EST_ChannelResampler resampler = {};

    EST_Attribute   attributes = {};
    enum EST_STATUS status = EST_STATUS_IDLE;
    bool            isInit = false;
    bool            isPlaying = false;
    bool            isAtEnd = false;
    bool            isRemoved = false;

    std::shared_ptr<EST_ChannelResampler> pitch = {};
    std::vector<EST_ChannelDataCallback>  callbacks;
};

struct EST_MemoryItem
{
    std::vector<float> data;
    int                pcmSize = 0;
    int                channels = 0;
    int                sampleRate = 0;
};

struct EST_Device
{
    int channels = 0;

    ma_context context = {};
    ma_device  device = {};

    std::vector<float>                   temporaryData;
    std::vector<float>                   processingData;
    std::vector<EST_ChannelDataCallback> callbacks;

    std::vector<std::shared_ptr<EST_Channel>> channel_arrays;
    std::string                               error;
    std::shared_ptr<std::mutex>               mutex;

    std::unordered_map<std::string, EST_MemoryItem> memory;
};

#endif