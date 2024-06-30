#ifndef __CHANNEL_INTERNAL_H_
#define __CHANNEL_INTERNAL_H_

#include "../Internal.h"

struct EST_Channel *InternalInit(EST_Device *device, std::string hash, int channels, int pcmSize, int sampleRate);
struct EST_Channel *InternalInitMemory(EST_Device *device, float *data, int channels, int pcmSize, int sampleRate);

#endif