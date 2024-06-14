#include "EncoderInternal.h"

EST_RESULT EST_EncoderExportFile(EST_Encoder *handle, enum EST_FILE_EXPORT type, char *filePath)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (memcmp(&handle->signature, EST_ENCODER_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("Invalid pointer magic");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!handle->data.size()) {
        EST_ErrorSetMessage("handle not renderer");
        return EST_ERROR_ENCODER_EMPTY;
    }

    if (type == EST_EXPORT_WAV) {
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,
            ma_format_s16,
            handle->channels,
            handle->decoder.outputSampleRate);

        ma_encoder encoder;
        ma_result  result = ma_encoder_init_file(filePath, &config, &encoder);
        if (result != MA_SUCCESS) {
            return EST_ERROR_INVALID_ARGUMENT;
        }

        int dataSize = handle->numOfPcmProcessed * handle->channels;

        // Convert data to signed int16
        std::vector<int16_t> data(dataSize);
        for (int i = 0; i < dataSize; i++) {
            float f = handle->data[i];
            f = std::clamp(f * 32768.0f, -32768.0f, 32767.0f);

            int16_t val = static_cast<int16_t>(f);
            data[i] = val;
        }

        ma_uint64 written = 0;
        result = ma_encoder_write_pcm_frames(&encoder, &data[0], handle->numOfPcmProcessed, &written);
        if (result != MA_SUCCESS) {
            return EST_ERROR_ENCODER_INVALID_WRITE;
        }

        ma_encoder_uninit(&encoder);
    } else {
        EST_ErrorSetMessage("Invalid export type or unsupported");
        return EST_ERROR_ENCODER_UNSUPPORTED;
    }

    return EST_OK;
}