#include "ChannelInternal.h"
#include "../../Utils/IO.h"

struct EST_Channel *EST_ChannelLoad(EST_Device *device, const char *filename)
{
    if (!device) {
        EST_ErrorSetMessage("EST_ChannelLoad: device is nullptr");
        return nullptr;
    }

    if (strnlen_s(filename, 256) == 0) {
        EST_ErrorSetMessage("EST_ChannelLoad: filename is empty");
        return nullptr;
    }

    std::string hash = HashFile(filename);
    auto        it = device->memory.find(hash);
    if (it != device->memory.end()) {
        return InternalInit(
            device,
            &it->second.data[0],
            it->second.channels,
            it->second.pcmSize,
            it->second.sampleRate);
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
        EST_ErrorSetMessage("EST_ChannelLoad: failed to load audio file");
        return nullptr;
    }

    ma_uint64 pcmSize = 0;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;

    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to get data format");
        return nullptr;
    }

    result = ma_decoder_get_available_frames(&decoder, &pcmSize);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_ChannelLoad: failed to get available frames");
        return nullptr;
    }

    std::vector<float> buffer(pcmSize * channels);
    ma_uint64          pcmToRead = pcmSize;
    while (pcmToRead > 0) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS) {
            EST_ErrorSetMessage("EST_ChannelLoad: failed to read pcm frames");
            return nullptr;
        }

        pcmToRead -= framesRead;
    }

    ma_decoder_uninit(&decoder);

    EST_MemoryItem item;
    item.data = buffer;
    item.pcmSize = (int)pcmSize;
    item.channels = channels;
    item.sampleRate = sampleRate;

    device->memory[hash] = std::move(item);

    return InternalInit(device, &device->memory[hash].data[0], channels, (int)pcmSize, sampleRate);
}

struct EST_Channel *EST_ChannelLoadFromMemory(EST_Device *device, const void *data, size_t size)
{
    if (!device) {
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: device is nullptr");
        return nullptr;
    }

    if (!data) {
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: data is nullptr");
        return nullptr;
    }

    if (size == 0) {
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: size is 0");
        return nullptr;
    }

    std::string hash = HashBuffer(data, size);
    auto        it = device->memory.find(hash);
    if (it != device->memory.end()) {
        return InternalInit(
            device,
            &it->second.data[0],
            it->second.channels,
            it->second.pcmSize,
            it->second.sampleRate);
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
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: failed to load audio file");
        return nullptr;
    }

    ma_uint64 pcmSize = 0;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;

    result = ma_decoder_get_data_format(&decoder, nullptr, &channels, &sampleRate, nullptr, 0);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: failed to get data format");
        return nullptr;
    }

    result = ma_decoder_get_available_frames(&decoder, &pcmSize);
    if (result != MA_SUCCESS) {
        EST_ErrorSetMessage("EST_ChannelLoadFromMemory: failed to get available frames");
        return nullptr;
    }

    std::vector<float> buffer(pcmSize * channels);
    ma_uint64          pcmToRead = pcmSize;
    while (pcmToRead > 0) {
        ma_uint64 framesRead = pcmToRead;
        result = ma_decoder_read_pcm_frames(&decoder, &buffer[0], framesRead, &framesRead);
        if (result != MA_SUCCESS) {
            EST_ErrorSetMessage("EST_ChannelLoadFromMemory: failed to read pcm frames");
            return nullptr;
        }

        pcmToRead -= framesRead;
    }

    ma_decoder_uninit(&decoder);

    EST_MemoryItem item;
    item.data = buffer;
    item.pcmSize = (int)pcmSize;
    item.channels = channels;
    item.sampleRate = sampleRate;

    device->memory[hash] = std::move(item);

    return InternalInit(device, &device->memory[hash].data[0], channels, (int)pcmSize, sampleRate);
}