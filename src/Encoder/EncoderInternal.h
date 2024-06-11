#ifndef __ENCODER_INTERNAL_H_
#define __ENCODER_INTERNAL_H_

#include <EstTypes.h>
#include <EstEncoder.h>
#include <EstAudio.h>

#include "../third-party/signalsmith-stretch/signalsmith-stretch.h"
#include "../third-party/miniaudio/miniaudio_decoders.h"
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

#endif