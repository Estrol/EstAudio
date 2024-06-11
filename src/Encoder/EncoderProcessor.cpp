#include "EncoderInternal.h"

EST_RESULT EST_EncoderSeek(EST_ENCODER_HANDLE handle, int index)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_seek_to_pcm_frame(&decoder->decoder, index);

    // If using timestretch, we need to process initial buffer
    if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
        decoder->processor->reset();

        int latency = decoder->processor->inputLatency() * 2;
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
        decoder->processor->process(convertedData, readed, outputProcess, readed);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderRender(EST_ENCODER_HANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_uint64 targetRead = static_cast<ma_uint64>(::floor(decoder->decoder.outputSampleRate * 0.01));

    decoder->data.clear();
    decoder->data.resize(0);
    decoder->processor->reset();
    decoder->processor->presetDefault(
        static_cast<int>(decoder->channels),
        static_cast<float>(decoder->decoder.outputSampleRate));
    decoder->numOfPcmProcessed = 0;

    int                bufferSize = static_cast<int>(::floor(targetRead * decoder->decoder.outputChannels * decoder->rate));
    std::vector<float> buffer(bufferSize);
    std::vector<float> temp(buffer.size());

    /*
     * This is bit tricky, as I want my encoder to support both resampler and timestretch
     *
     * My workflow is:
     * 1. Process resampler
     * 2. -=- timestretch
     * 3. -=- vol/pan
     * 4. -=- user-callback
     * 5. -=- clipping
     */

    EST_EncoderSeek(handle, 0);

    while (true) {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        std::fill(temp.begin(), temp.end(), 0.0f);

        ma_uint64 targetThisIteration = targetRead;
        if (decoder->rate != 1.0f) {
            ma_resampler_get_required_input_frame_count(
                &decoder->calculator,
                targetRead,
                &targetThisIteration);
        }

        ma_uint64 readed = 0;
        auto      result = ma_decoder_read_pcm_frames(&decoder->decoder, &buffer[0], targetThisIteration, &readed);
        if (result != MA_SUCCESS || readed == 0) {
            break;
        }

        if (decoder->channels != static_cast<int>(decoder->decoder.outputChannels)) {
            ma_channel_converter_process_pcm_frames(&decoder->converter, &temp[0], &buffer[0], readed);

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::copy(&temp[0], &temp[0] + readed * decoder->channels, &buffer[0]);
        }

        // Effect processing
        {
            if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
                decoder->processor->process(
                    buffer,
                    static_cast<int>(readed),
                    temp,
                    static_cast<int>(targetRead));

                readed = targetRead;

                std::copy(&temp[0], &temp[0] + readed * decoder->channels, &buffer[0]);
            }

            result = ma_gainer_process_pcm_frames(&decoder->gainer, &temp[0], &buffer[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&decoder->panner, &buffer[0], &temp[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }
        }

        int totalReadedThisIteration = static_cast<int>(readed);

        // User-default callback
        if (decoder->callback) {
            decoder->callback(handle, decoder->userData, &buffer[0], totalReadedThisIteration);
        }

        int sizeToCopy = totalReadedThisIteration * decoder->channels;
        std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(decoder->data));

        decoder->numOfPcmProcessed += totalReadedThisIteration;
    }

    // If using timestretch, we need to process the remaining buffer
    if (decoder->rate != 1.0f || decoder->pitch != 1.0f) {
        int outputBuffer = decoder->processor->outputLatency();

        int currentCursor = 0;
        while (currentCursor < outputBuffer) {
            ma_uint64 frameToRead = targetRead;

            if (currentCursor + targetRead > outputBuffer) {
                frameToRead = outputBuffer - currentCursor;
            }

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::fill(temp.begin(), temp.end(), 0.0f);

            decoder->processor->process(
                temp,
                0,
                buffer,
                static_cast<int>(frameToRead));

            auto result = ma_gainer_process_pcm_frames(&decoder->gainer, &temp[0], &buffer[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&decoder->panner, &buffer[0], &temp[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            if (decoder->callback) {
                decoder->callback(handle, decoder->userData, &buffer[0], static_cast<int>(frameToRead));
            }

            int sizeToCopy = static_cast<int>(frameToRead) * decoder->channels;
            std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(decoder->data));

            currentCursor += static_cast<int>(frameToRead);
            decoder->numOfPcmProcessed += static_cast<int>(frameToRead);
        }
    }

    // Clip the audio data to prevent distortion
    for (int i = 0; i < decoder->data.size(); i++) {
        decoder->data[i] = std::clamp(decoder->data[i], -1.0f, 1.0f);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetData(EST_ENCODER_HANDLE handle, void *data, int *size)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->numOfPcmProcessed > 0) {
        float *pData = static_cast<float *>(data);
        int    numOfFloatBytes = decoder->numOfPcmProcessed * decoder->channels;

        std::copy(
            &decoder->data[0],
            &decoder->data[0] + numOfFloatBytes,
            pData);

        *size = decoder->numOfPcmProcessed;
    } else {
        return EST_ERROR_ENCODER_EMPTY;
    }

    return EST_OK;
}

EST_RESULT EST_EncoderFlushData(EST_ENCODER_HANDLE handle)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    decoder->data.resize(0);
    decoder->numOfPcmProcessed = 0;

    return EST_OK;
}

EST_RESULT EST_EncoderGetAvailableDataSize(EST_ENCODER_HANDLE handle, int *size)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    *size = decoder->numOfPcmProcessed;

    return EST_OK;
}

EST_RESULT EST_EncoderGetSample(EST_ENCODER_HANDLE handle, EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE *outSample)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (devhandle == nullptr) {
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (decoder->numOfPcmProcessed == 0) {
        auto renderResult = EST_EncoderRender(handle);
        if (renderResult != EST_OK) {
            return renderResult;
        }
    }

    int maxChannels = decoder->channels;

    auto result = EST_SampleLoadRawPCM(devhandle, decoder->data.data(), decoder->numOfPcmProcessed, maxChannels, 44100, outSample);
    if (result != EST_OK) {
        return result;
    }

    return EST_OK;
}