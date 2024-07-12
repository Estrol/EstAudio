#ifndef __AUDIO_INTERNAL_H_
#define __AUDIO_INTERNAL_H_

#include <EstAudio.h>
#include "../Unknown.h"

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

struct EST_DataCallback
{
    EST_Unknown base = {
        EST_UNKNOWN_DATA_CALLBACK
    };

    EST_DATA_CALLBACK callback;
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

constexpr char EST_SAMPLE_MAGIC[5] = "ESTS";

struct EST_Sample
{
    EST_Unknown base = {
        EST_UNKNOWN_SAMPLE
    };

    int channels = 0;
    int sampleRate = 0;
    int pcmSize = 0;

    std::vector<float> data = {};
    std::vector<struct EST_Channel *> channelsPlaying = {};
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

constexpr char EST_CHANNEL_MAGIC[5] = "ESTC";

struct EST_Channel
{
    EST_Unknown base = {
        EST_UNKNOWN_CHANNEL
    };

    EST_Device *device = nullptr;
    
    int         channels = 0;
    ma_uint32   sampleRate = 0;
    std::string memoryHash = "";

    ma_audio_buffer      buffer = {};
    ma_panner            panner = {};
    ma_gainer            gainer = {};
    ma_channel_converter converter = {};

    EST_Attribute   attributes = {};
    enum EST_STATUS status = EST_STATUS_IDLE;
    bool            isInit = false;
    bool            isPlaying = false;
    bool            isAtEnd = false;
    bool            isRemoved = false;

    std::shared_ptr<EST_ChannelResampler> pitch = {};
    std::vector<EST_DataCallback>  callbacks;
};

struct EST_ChannelDestructor
{
    inline void operator()(EST_Channel *channel) const
    {
        if (!channel->isInit) {
            return;
        }

        ma_audio_buffer_uninit(&channel->buffer);
        ma_channel_converter_uninit(&channel->converter, nullptr);
        ma_gainer_uninit(&channel->gainer, nullptr);
    }
};

const float EST_MEMORY_TIMEOUT = 15.0f;

struct EST_MemoryItem
{
    std::vector<float> data;
    int                pcmSize = 0;
    int                channels = 0;
    int                sampleRate = 0;

    int   useCount = 0;
    float useTimeout = 0.0f; // in seconds, only increment when none of the channels are using this memory
};

struct EST_Device
{
    EST_Unknown base = {
        EST_UNKNOWN_DEVICE
    };

    int                                                         channels = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> time = std::chrono::high_resolution_clock::now();

    ma_context context = {};
    ma_device  device = {};

    std::vector<float>                   temporaryData;
    std::vector<float>                   processingData;
    std::vector<EST_DataCallback> callbacks;

    std::vector<std::shared_ptr<EST_Channel>> channel_arrays;
    std::vector<EST_Channel*> guest_channel_arrays;

    std::string                               error;
    std::shared_ptr<std::mutex>               mutex;

    std::unordered_map<std::string, EST_MemoryItem> memory;
};

#endif