#include "EncoderInternal.h"
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4996) // Disable warning for fopen
#endif

EST_RESULT EST_EncoderExportFile(EST_Encoder *handle, enum EST_FILE_EXPORT type, char *filePath)
{
    if (!handle) {
        EST_ErrorSetMessage("Invalid handle");
        return EST_ERROR_INVALID_ARGUMENT;
    }

    EST_Unknown *unknown = (EST_Unknown *)handle;
    if (unknown->type != EST_UNKNOWN_ENCODER) {
        auto msg = std::format("Invalid handle: {} (Expect: {})", EST_UnknownTypeToString(unknown->type), EST_UnknownTypeToString(EST_UNKNOWN_ENCODER));
        EST_ErrorSetMessage(msg.c_str());
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
    } else if (type == EST_EXPORT_OGG)
    {
        if (handle->channels != 1 && handle->channels != 2) {
            EST_ErrorSetMessage("EST_EXPORT_OGG only support mono or stereo channel");
            return EST_ERROR_ENCODER_INVALID_OPERATION;
        }

        vorbis_info vi;
        vorbis_info_init(&vi);

        if (vorbis_encode_init_vbr(&vi, handle->channels, handle->decoder.outputSampleRate, 1.0f) != 0) {
            EST_ErrorSetMessage("Failed to initialize vorbis encoder");
            return EST_ERROR_ENCODER_INVALID_OPERATION;
        }

        FILE *file = fopen(filePath, "wb");
        if (!file) {
            EST_ErrorSetMessage("Failed to open file");
            return EST_ERROR_ENCODER_INVALID_OPERATION;
        }

        vorbis_comment vc;
        vorbis_comment_init(&vc);
        vorbis_comment_add_tag(&vc, "ENCODER", "EST_Encoder::Export::libvorbis");

        vorbis_dsp_state vd;
        vorbis_block vb;  

        vorbis_analysis_init(&vd, &vi);
        vorbis_block_init(&vd, &vb);

        ogg_stream_state os;
        srand((uint32_t)time(NULL));
        ogg_stream_init(&os, rand());

        ogg_packet header, header_comm, header_code;
        vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
        ogg_stream_packetin(&os, &header);
        ogg_stream_packetin(&os, &header_comm);
        ogg_stream_packetin(&os, &header_code);

        ogg_page og;
        while (ogg_stream_flush(&os, &og)) {
            fwrite(og.header, 1, og.header_len, file);
            fwrite(og.body, 1, og.body_len, file);
        }

        int chunkSize = 1024;
        for (int i = 0; i < handle->numOfPcmProcessed; i += chunkSize) {
            float **buffer = vorbis_analysis_buffer(&vd, chunkSize);
            for (int j = 0; j < chunkSize; j++) {
                int channelSize = handle->channels;

                if (channelSize == 2) {
                    if (i + j < handle->numOfPcmProcessed) {
                        buffer[0][j] = handle->data[2 * (i + j)];       // Left channel
                        buffer[1][j] = handle->data[2 * (i + j) + 1];   // Right channel
                    } else {
                        buffer[0][j] = 0.f;
                        buffer[1][j] = 0.f;
                    }
                } else if (channelSize == 1) {
                    if (i + j < handle->numOfPcmProcessed) {
                        buffer[0][j] = buffer[1][j] = handle->data[i + j];
                    } else {
                        buffer[0][j] = buffer[1][j] = 0.f;
                    }
                }
            }

            vorbis_analysis_wrote(&vd, chunkSize);

            while (vorbis_analysis_blockout(&vd, &vb) == 1) {
                vorbis_analysis(&vb, NULL);
                vorbis_bitrate_addblock(&vb);

                ogg_packet op;
                while (vorbis_bitrate_flushpacket(&vd, &op)) {
                    ogg_stream_packetin(&os, &op);
                    while (ogg_stream_pageout(&os, &og)) {
                        fwrite(og.header, 1, og.header_len, file);
                        fwrite(og.body, 1, og.body_len, file);
                    }
                }
            }
        }

        vorbis_analysis_wrote(&vd, 0);

        while (vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);

            ogg_packet op;
            while (vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);

                while (ogg_stream_pageout(&os, &og)) {
                    fwrite(og.header, 1, og.header_len, file);
                    fwrite(og.body, 1, og.body_len, file);
                }
            }
        }

        ogg_stream_clear(&os);
        vorbis_block_clear(&vb);
        vorbis_dsp_clear(&vd);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
        fclose(file);
    }

    return EST_OK;
}