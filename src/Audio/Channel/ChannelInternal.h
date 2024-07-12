#ifndef __CHANNEL_INTERNAL_H_
#define __CHANNEL_INTERNAL_H_

#include "../Internal.h"

struct EST_Channel *ChannelInternalInit(EST_Device *device, std::string hash, int channels, int pcmSize, int sampleRate);
struct EST_Channel *ChannelInternalInitMemory(EST_Device *device, float *data, int channels, int pcmSize, int sampleRate);
void                ChannelInternalFree(EST_Channel *channel);
EST_RESULT          ChannelInternalFFTGetData(EST_Channel *channel, float *data, int size, bool individual = false);   
EST_RESULT          ChannelInternalGetData(EST_Channel *channel, float *data, int size);
EST_RESULT          ChannelInternalSetPositionMS(EST_Channel *channel, float ms);

#endif