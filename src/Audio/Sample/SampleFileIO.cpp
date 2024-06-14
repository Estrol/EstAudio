#include "../Internal.h"
#include "../../Utils/IO.h"

EST_Sample *EST_SampleLoad(const char *filename)
{
    if (!filename) {
        EST_ErrorSetMessage("EST_SampleLoad: filename is nullptr");
        return nullptr;
    }

    if (strnlen_s(filename, 256) == 0) {
        EST_ErrorSetMessage("EST_SampleLoad: filename is empty");
        return nullptr;
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
    ma_result  result = ma_decoder_init_file(filename, &config, &decoder);

    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoad: failed to load audio file");
        return nullptr;
    }

    ma_uint64 pcmSize = 0;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;

    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoad: failed to get data format");
        return nullptr;
    }

    result = ma_decoder_get_available_frames(&decoder, &pcmSize);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoad: failed to get available frames");
        return nullptr;
    }

    std::vector<float> buffer(pcmSize * channels);
    ma_uint64          pcmToRead = pcmSize;
    while (pcmToRead > 0) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS) {
            EST_ErrorSetMessage("EST_SampleLoad: failed to read pcm frames");
            return nullptr;
        }

        pcmToRead -= framesRead;
    }

    ma_decoder_uninit(&decoder);

    EST_Sample *sample = new EST_Sample;
    sample->channels = channels;
    sample->sampleRate = sampleRate;
    sample->pcmSize = (int)pcmSize;
    sample->data = buffer;

    return sample;
}

EST_Sample *EST_SampleLoadFromMemory(const void *data, size_t size)
{
    if (!data) {
        EST_ErrorSetMessage("EST_SampleLoadFromMemory: data is nullptr");
        return nullptr;
    }

    if (size == 0) {
        EST_ErrorSetMessage("EST_SampleLoadFromMemory: size is 0");
        return nullptr;
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
    ma_result  result = ma_decoder_init_memory(data, size, &config, &decoder);

    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to load audio file");
        return nullptr;
    }

    ma_uint64 pcmSize = 0;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;

    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to get data format");
        return nullptr;
    }

    result = ma_decoder_get_available_frames(&decoder, &pcmSize);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to get available frames");
        return nullptr;
    }

    std::vector<float> buffer(pcmSize * channels);
    ma_uint64          pcmToRead = pcmSize;
    while (pcmToRead > 0) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS) {
            EST_ErrorSetMessage("EST_SampleLoadFromMemory: failed to read pcm frames");
            return nullptr;
        }

        pcmToRead -= framesRead;
    }

    ma_decoder_uninit(&decoder);

    EST_Sample *sample = new EST_Sample;
    sample->channels = channels;
    sample->sampleRate = sampleRate;
    sample->pcmSize = (int)pcmSize;
    sample->data = buffer;

    return sample;
}

EST_RESULT EST_SampleFree(EST_Sample *sample)
{
    if (!sample) {
        EST_ErrorSetMessage("EST_SampleFree: sample is nullptr");
        return EST_ERROR;
    }

    if (memcmp(&sample->signature, EST_SAMPLE_MAGIC, 5) != 0) {
        EST_ErrorSetMessage("EST_SampleFree: invalid pointer magic");
        return EST_ERROR;
    }

    memset((void *)sample->signature, 0, 5);
    delete sample;
    return EST_OK;
}