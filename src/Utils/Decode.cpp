/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#include "../third-party/miniaudio/miniaudio.h"
#include "../third-party/miniaudio/miniaudio_decoders.h"

#include <vector>
#include <fstream>
#include <filesystem>

struct EST_AudioFile
{
    bool               success = false;
    std::string        message;
    std::vector<float> data;
};

EST_AudioFile LoadAudioData(const char *path)
{
    EST_AudioFile audioFile = { false, "", {} };

    if (!std::filesystem::exists(path)) {
        audioFile.message = "LoadAudioData: File does not exist";
        return audioFile;
    }

    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 2, 44100);

    ma_decoding_backend_vtable *pCustomBackendVTables[] = {
        &g_ma_decoding_backend_vtable_libvorbis,
        &g_ma_decoding_backend_vtable_libopus
    };

    config.pCustomBackendUserData = NULL;
    config.ppCustomBackendVTables = pCustomBackendVTables;
    config.customBackendCount = sizeof(pCustomBackendVTables) / sizeof(pCustomBackendVTables[0]);

    ma_decoder decoder;
    ma_result  result = ma_decoder_init_file(path, &config, &decoder);
    if (result != MA_SUCCESS) {
        audioFile.message = "LoadAudioData: Failed to load audio file";
        return audioFile;
    }

    ma_uint64 framesAvailable = 0;
    result = ma_decoder_get_available_frames(&decoder, &framesAvailable);

    if (result != MA_SUCCESS) {
        audioFile.message = "LoadAudioData: Failed to get available frames";
        return audioFile;
    }

    if (framesAvailable <= 0) {
        audioFile.message = "LoadAudioData: No frames available";
        return audioFile;
    }

    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);

    if (result != MA_SUCCESS) {
        audioFile.message = "LoadAudioData: Failed to get data format";
        return audioFile;
    }

    audioFile.data.resize(framesAvailable * channels);
    result = ma_decoder_read_pcm_frames(&decoder, audioFile.data.data(), framesAvailable, &framesAvailable);
    if (result != MA_SUCCESS) {
        ma_decoder_uninit(&decoder);

        audioFile.message = "LoadAudioData: Failed to read pcm frames";
        return audioFile;
    }

    ma_decoder_uninit(&decoder);

    audioFile.success = true;
    return audioFile;
}