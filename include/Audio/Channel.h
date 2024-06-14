#ifndef __CHANNEL_H_
#define __CHANNEL_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

EST_API struct EST_Channel *EST_ChannelLoad(EST_Device *device, const char *filename);

EST_API struct EST_Channel *EST_ChannelLoadFromMemory(EST_Device *device, const void *data, size_t size);

EST_API struct EST_Channel *EST_SampleGetChannel(EST_Device *device, EST_Sample *handle);

EST_API int EST_SampleGetChannels(EST_Device *device, EST_Sample *handle, int howManyChannelsToCreated, EST_Channel **out);

EST_API struct EST_Channel *EST_EncoderGetChannel(EST_Device *device, EST_Encoder *handle);

EST_API int EST_EncoderGetChannels(EST_Device *device, EST_Encoder *handle, int howManyChannelsToCreated, EST_Channel **out);

EST_API enum EST_RESULT EST_ChannelPlay(EST_Device *device, EST_Channel *handle, EST_BOOL restart);

EST_API enum EST_RESULT EST_ChannelPause(EST_Device *device, EST_Channel *handle);

EST_API enum EST_RESULT EST_ChannelStop(EST_Device *device, EST_Channel *handle);

EST_API enum EST_RESULT EST_ChannelFree(EST_Device *device, EST_Channel *handle);

EST_API enum EST_BOOL EST_ChannelIsPlaying(EST_Device *device, EST_Channel *handle);

EST_API enum EST_RESULT EST_ChannelSetAttribute(EST_Device *device, EST_Channel *handle, est_attribute_value *value);

EST_API enum EST_RESULT EST_ChannelGetAttribute(EST_Device *device, EST_Channel *handle, est_attribute_value *value);

#if __cplusplus
}
#endif

#endif