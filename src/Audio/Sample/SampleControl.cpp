#include "SampleInternal.h"

EST_RESULT EST_SampleSeek(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, int index)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    auto decoder = GetSample(device, handle);
    if (!decoder) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_seek_to_pcm_frame(&decoder->decoder, index);

    // If using timestretch, we need to process initial buffer
    if (decoder->attributes.rate != 1.0f || decoder->attributes.pitch != 1.0f) {
        decoder->pitch->processor->reset();

        int latency = decoder->pitch->processor->inputLatency() * 2;
        int readed = 0;

        std::vector<float> convertedData(latency * decoder->channels);

        if (decoder->channels != (int)decoder->decoder.outputChannels) {
            std::vector<float> encoderData(latency * decoder->decoder.outputChannels);

            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&decoder->decoder, &encoderData[0], latency, &ma_readed);

            ma_channel_converter_process_pcm_frames(&decoder->converter, &convertedData[0], &encoderData[0], ma_readed);

            readed = static_cast<int>(ma_readed);
        } else {
            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&decoder->decoder, &convertedData[0], latency, &ma_readed);

            readed = static_cast<int>(ma_readed);
        }

        std::vector<float> outputProcess(convertedData.size());
        decoder->pitch->processor->process(convertedData, readed, outputProcess, readed);
    }

    return EST_OK;
}

EST_RESULT EST_SamplePlay(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    it->isAtEnd = false;
    it->isPlaying = true;
    it->pitch->processor->reset();

    if (it->rawAudio) {
        ma_audio_buffer_seek_to_pcm_frame(&it->rawAudio->decoder, 0);
    } else {
        ma_decoder_seek_to_pcm_frame(&it->decoder, 0);
    }

    return EST_OK;
}

EST_RESULT EST_SampleStop(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    it->isPlaying = false;
    if (it->rawAudio) {
        ma_audio_buffer_seek_to_pcm_frame(&it->rawAudio->decoder, 0);
    } else {
        ma_decoder_seek_to_pcm_frame(&it->decoder, 0);
    }

    return EST_OK;
}

EST_RESULT EST_SampleGetStatus(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle, enum EST_STATUS *value)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);

    if (!device) {
        EST_SetError("No context");
        return EST_ERROR_INVALID_STATE;
    }

    auto it = GetSample(device, handle);
    if (!it) {
        EST_SetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (it->isPlaying) {
        *value = EST_STATUS_PLAYING;
    } else {
        if (it->isAtEnd) {
            *value = EST_STATUS_AT_END;
        } else {
            *value = EST_STATUS_IDLE;
        }
    }

    return EST_OK;
}