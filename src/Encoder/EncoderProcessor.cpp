#include "EncoderInternal.h"

EST_RESULT EST_EncoderSeek(EST_Encoder *handle, int index)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_decoder_seek_to_pcm_frame(&handle->decoder, index);

    // If using timestretch, we need to process initial buffer
    if (handle->rate != 1.0f || handle->pitch != 1.0f) {
        handle->processor->reset();

        int latency = handle->processor->inputLatency() * 2;
        int readed = 0;

        std::vector<float> convertedData(latency * handle->channels);

        if (handle->channels != (int)handle->decoder.outputChannels) {
            std::vector<float> encoderData(latency * handle->decoder.outputChannels);

            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&handle->decoder, &encoderData[0], latency, &ma_readed);

            ma_channel_converter_process_pcm_frames(&handle->converter, &convertedData[0], &encoderData[0], ma_readed);

            readed = static_cast<int>(ma_readed);
        } else {
            ma_uint64 ma_readed = 0;
            ma_decoder_read_pcm_frames(&handle->decoder, &convertedData[0], latency, &ma_readed);

            readed = static_cast<int>(ma_readed);
        }

        std::vector<float> outputProcess(convertedData.size());
        handle->processor->process(convertedData, readed, outputProcess, readed);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderRender(EST_Encoder *handle)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    ma_uint64 targetRead = static_cast<ma_uint64>(::floor(handle->decoder.outputSampleRate * 0.01));

    handle->data.clear();
    handle->data.resize(0);
    handle->processor->reset();
    handle->processor->presetDefault(
        static_cast<int>(handle->channels),
        static_cast<float>(handle->decoder.outputSampleRate));
    handle->numOfPcmProcessed = 0;

    int                bufferSize = static_cast<int>(::floor(targetRead * handle->decoder.outputChannels * handle->rate));
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
        if (handle->rate != 1.0f) {
            ma_resampler_get_required_input_frame_count(
                &handle->calculator,
                targetRead,
                &targetThisIteration);
        }

        ma_uint64 readed = 0;
        auto      result = ma_decoder_read_pcm_frames(&handle->decoder, &buffer[0], targetThisIteration, &readed);
        if (result != MA_SUCCESS || readed == 0) {
            break;
        }

        if (handle->channels != static_cast<int>(handle->decoder.outputChannels)) {
            ma_channel_converter_process_pcm_frames(&handle->converter, &temp[0], &buffer[0], readed);

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::copy(&temp[0], &temp[0] + readed * handle->channels, &buffer[0]);
        }

        // Effect processing
        {
            if (handle->rate != 1.0f || handle->pitch != 1.0f) {
                handle->processor->process(
                    buffer,
                    static_cast<int>(readed),
                    temp,
                    static_cast<int>(targetRead));

                readed = targetRead;

                std::copy(&temp[0], &temp[0] + readed * handle->channels, &buffer[0]);
            }

            result = ma_gainer_process_pcm_frames(&handle->gainer, &temp[0], &buffer[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&handle->panner, &buffer[0], &temp[0], readed);
            if (result != MA_SUCCESS) {
                break;
            }
        }

        int totalReadedThisIteration = static_cast<int>(readed);

        // User-default callback
        if (handle->callback) {
            handle->callback(handle, handle->userData, &buffer[0], totalReadedThisIteration);
        }

        int sizeToCopy = totalReadedThisIteration * handle->channels;
        std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(handle->data));

        handle->numOfPcmProcessed += totalReadedThisIteration;
    }

    // If using timestretch, we need to process the remaining buffer
    if (handle->rate != 1.0f || handle->pitch != 1.0f) {
        int outputBuffer = handle->processor->outputLatency();

        int currentCursor = 0;
        while (currentCursor < outputBuffer) {
            ma_uint64 frameToRead = targetRead;

            if (currentCursor + targetRead > outputBuffer) {
                frameToRead = outputBuffer - currentCursor;
            }

            std::fill(buffer.begin(), buffer.end(), 0.0f);
            std::fill(temp.begin(), temp.end(), 0.0f);

            handle->processor->process(
                temp,
                0,
                buffer,
                static_cast<int>(frameToRead));

            auto result = ma_gainer_process_pcm_frames(&handle->gainer, &temp[0], &buffer[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            result = ma_panner_process_pcm_frames(&handle->panner, &buffer[0], &temp[0], frameToRead);
            if (result != MA_SUCCESS) {
                break;
            }

            if (handle->callback) {
                handle->callback(handle, handle->userData, &buffer[0], static_cast<int>(frameToRead));
            }

            int sizeToCopy = static_cast<int>(frameToRead) * handle->channels;
            std::copy(&buffer[0], &buffer[0] + sizeToCopy, std::back_inserter(handle->data));

            currentCursor += static_cast<int>(frameToRead);
            handle->numOfPcmProcessed += static_cast<int>(frameToRead);
        }
    }

    // Clip the audio data to prevent distortion
    for (int i = 0; i < handle->data.size(); i++) {
        handle->data[i] = std::clamp(handle->data[i], -1.0f, 1.0f);
    }

    return EST_OK;
}

EST_RESULT EST_EncoderGetData(EST_Encoder *handle, void *data, int *size)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (handle->numOfPcmProcessed > 0) {
        float *pData = static_cast<float *>(data);
        int    numOfFloatBytes = handle->numOfPcmProcessed * handle->channels;

        std::copy(
            &handle->data[0],
            &handle->data[0] + numOfFloatBytes,
            pData);

        *size = handle->numOfPcmProcessed;
    } else {
        return EST_ERROR_ENCODER_EMPTY;
    }

    return EST_OK;
}

EST_RESULT EST_EncoderFlushData(EST_Encoder *handle)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    handle->data.resize(0);
    handle->numOfPcmProcessed = 0;

    return EST_OK;
}

EST_RESULT EST_EncoderGetAvailableDataSize(EST_Encoder *handle, int *size)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    *size = handle->numOfPcmProcessed;

    return EST_OK;
}