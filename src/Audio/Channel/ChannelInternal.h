#ifndef __CHANNEL_INTERNAL_H_
#define __CHANNEL_INTERNAL_H_

#include "../Internal.h"

struct EST_Channel *InternalInit(EST_Device *device, float *data_pointer, int channels, int pcmSize, int sampleRate);

#endif