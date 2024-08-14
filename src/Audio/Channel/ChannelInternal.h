/**
 * Copyright (c) 2024 Estrol Mendex.
 * See the LICENSE file for copying permission.
 */

#ifndef __CHANNEL_INTERNAL_H_
#define __CHANNEL_INTERNAL_H_

#include "../Internal.h"

struct EST_Channel *ChannelInternalInit(EST_Device *device, std::string hash, int channels, int pcmSize, int sampleRate, EST_FX* fx);
struct EST_Channel *ChannelInternalInitMemory(EST_Device *device, float *data, int channels, int pcmSize, int sampleRate, EST_FX* fx);
void                ChannelInternalFree(EST_Channel *channel);
int                 ChannelInternalFFTGetData(EST_Channel *channel, float *data, int size, bool individual = false);
int                 ChannelInternalGetData(EST_Channel *channel, float *data, int size);
EST_RESULT          ChannelInternalSetPositionMS(EST_Channel *channel, float ms);

#endif