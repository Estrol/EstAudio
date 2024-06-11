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

struct EST_AudioCallback
{
    est_audio_callback callback;
    void              *userdata;
};

struct EST_AudioResampler
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

struct EST_RawAudio
{
    ma_audio_buffer decoder = {};

    std::vector<float> PCMData;
    int                PCMSize = 0;
};

struct EST_AudioSample
{
    int channels = 0;

    bool isInit = false;
    bool isPlaying = false;
    bool isAtEnd = false;
    bool isRemoved = false;

    EST_Attribute                 attributes = {};
    std::shared_ptr<EST_RawAudio> rawAudio;

    ma_decoder           decoder = {};
    ma_panner            panner = {};
    ma_gainer            gainer = {};
    ma_channel_converter converter = {};

    std::shared_ptr<EST_AudioResampler> pitch = {};
    std::vector<EST_AudioCallback>      callbacks;
};

struct EST_AudioDestructor
{
    inline void operator()(EST_AudioSample *sample) const
    {
        if (!sample->isInit) {
            return;
        }

        if (sample->rawAudio) {
            ma_audio_buffer_uninit(&sample->rawAudio->decoder);
        } else {
            ma_decoder_uninit(&sample->decoder);
        }

        ma_channel_converter_uninit(&sample->converter, nullptr);
        ma_gainer_uninit(&sample->gainer, nullptr);
    }
};

struct EST_ResamplerDestructor
{
    inline void operator()(EST_AudioResampler *sample) const
    {
        if (!sample->isInit) {
            return;
        }

        ma_resampler_uninit(&sample->resampler, nullptr);
        sample->processor->reset();
    }
};

struct EST_AudioDevice
{
    int channels = 0;

    ma_context context = {};
    ma_device  device = {};

    std::vector<float>             temporaryData;
    std::vector<float>             processingData;
    std::vector<EST_AudioCallback> callbacks;

    std::unordered_map<EST_AUDIO_HANDLE, std::shared_ptr<EST_AudioSample>> samples;
    std::string                                                            error;
    std::shared_ptr<std::mutex>                                            mutex;

    EST_AUDIO_HANDLE HandleCounter = 0;
};

#endif