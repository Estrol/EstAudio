#include "SampleInternal.h"

std::shared_ptr<EST_AudioSample> GetSample(EST_DEVICE_HANDLE devhandle, EST_AUDIO_HANDLE handle)
{
    auto device = reinterpret_cast<EST_AudioDevice *>(devhandle);
    if (!device) {
        return nullptr;
    }

    auto it = device->samples.find(handle);
    if (it == device->samples.end()) {
        return nullptr;
    }

    return it->second;
}