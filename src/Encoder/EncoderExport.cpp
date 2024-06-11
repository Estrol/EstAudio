#include "EncoderInternal.h"

EST_RESULT EST_EncoderExportFile(EST_ENCODER_HANDLE handle, enum EST_FILE_EXPORT type, char *filePath)
{
    auto decoder = reinterpret_cast<EST_Encoder *>(handle);
    if (!decoder || decoder->Signature != kESTEncoderSignature) {
        EST_EncoderSetError("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    if (!decoder->data.size()) {
        EST_EncoderSetError("Decoder not renderer");
        return EST_ERROR_ENCODER_EMPTY;
    }

    if (type == EST_EXPORT_WAV) {
        ma_encoder_config config = ma_encoder_config_init(
            ma_encoding_format_wav,
            ma_format_s16,
            decoder->channels,
            decoder->decoder.outputSampleRate);

        ma_encoder encoder;
        ma_result  result = ma_encoder_init_file(filePath, &config, &encoder);
        if (result != MA_SUCCESS) {
            return EST_ERROR_INVALID_ARGUMENT;
        }

        int dataSize = decoder->numOfPcmProcessed * decoder->channels;

        // Convert data to signed int16
        std::vector<int16_t> data(dataSize);
        for (int i = 0; i < dataSize; i++) {
            float f = decoder->data[i];
            f = std::clamp(f * 32768.0f, -32768.0f, 32767.0f);

            int16_t val = static_cast<int16_t>(f);
            data[i] = val;
        }

        ma_uint64 written = 0;
        result = ma_encoder_write_pcm_frames(&encoder, &data[0], decoder->numOfPcmProcessed, &written);
        if (result != MA_SUCCESS) {
            return EST_ERROR_ENCODER_INVALID_WRITE;
        }

        ma_encoder_uninit(&encoder);
    } else {
        EST_EncoderSetError("Invalid export type or unsupported");
        return EST_ERROR_ENCODER_UNSUPPORTED;
    }

    return EST_OK;
}