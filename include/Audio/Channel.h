#ifndef __CHANNEL_H_
#define __CHANNEL_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

/**
 * Create audio playback channel from file
 *
 * @param device The device to create the channel
 * @param filename The path to the file
 *
 * @return `EST_Channel*` The channel object
 */
EST_API struct EST_Channel *EST_ChannelLoad(EST_Device *device, const char *filename);

/**
 * Create audio playback channel from memory
 *
 * @param device The device to create the channel
 * @param data The data of the file
 * @param size The size of the data
 *
 * @return `EST_Channel*` The channel object
 */
EST_API struct EST_Channel *EST_ChannelLoadFromMemory(EST_Device *device, const void *data, size_t size);

/**
 * Create audio playback channel from `EST_Sample`
 *
 * @param device The device to create the channel
 * @param sample The `EST_Sample` object
 *
 * @return `EST_Channel*` The channel object
 */
EST_API struct EST_Channel *EST_SampleGetChannel(EST_Device *device, EST_Sample *handle);

/**
 * Create N audio playback channel from `EST_Sample`
 *
 * @param device The device to create the channel
 * @param handle The `EST_Sample` object
 * @param howManyChannelsToCreated The number of channels to create
 * @param out The output array of channels
 *
 * @return `int` The number of channels created
 */
EST_API int EST_SampleGetChannels(EST_Device *device, EST_Sample *handle, int howManyChannelsToCreated, EST_Channel **out);

/**
 * Create audio playback channel from `EST_Encoder`
 *
 * @param device The device to create the channel
 * @param handle The `EST_Encoder` object
 *
 * @return `EST_Channel*` The channel object
 */
EST_API struct EST_Channel *EST_EncoderGetChannel(EST_Device *device, EST_Encoder *handle);

/**
 * Create N audio playback channel from `EST_Encoder`
 *
 * @param device The device to create the channel
 * @param handle The `EST_Encoder` object
 * @param howManyChannelsToCreated The number of channels to create
 * @param out The output array of channels
 *
 * @return `int` The number of channels created
 */
EST_API int EST_EncoderGetChannels(EST_Device *device, EST_Encoder *handle, int howManyChannelsToCreated, EST_Channel **out);

/**
 * Play the channel
 *
 * @param handle The channel object
 * @param restart If true, the channel will be restarted
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelPlay(EST_Channel *handle, EST_BOOL restart);

/**
 * Pause the channel
 *
 * @param handle The channel object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelPause(EST_Channel *handle);

/**
 * Stop the channel
 *
 * @param handle The channel object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelStop(EST_Channel *handle);

/**
 * Free the channel object
 *
 * @param handle The channel object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelFree(EST_Channel *handle);

/**
 * Seek the channel to position
 *
 * @param handle The channel object
 * @param type The type of the position, see `EST_CHANNEL_POSITION_TYPE` for more information
 * @param position The position to seek
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelSeekPosition(EST_Channel *handle, enum EST_CHANNEL_POSITION_TYPE type, float position);

/**
 * Check if the channel is playing
 *
 * @param handle The channel object
 *
 * @return `EST_BOOL` The result of the operation
 */
EST_API enum EST_BOOL EST_ChannelIsPlaying(EST_Channel *handle);

/**
 * Set the attribute of the channel
 *
 * @param handle The channel object
 * @param value See `est_attribute_value`
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelSetAttribute(EST_Channel *handle, est_attribute_value *value);

/**
 * Get the attribute of the channel
 *
 * @param handle The channel object
 * @param value See `est_attribute_value`
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelGetAttribute(EST_Channel *handle, est_attribute_value *value);

/**
 * Add callback to the channel, used for DSP processing
 *
 * @param handle The channel object
 * @param callback The callback function
 * @param userData The user data
 *
 * @return `EST_DataCallback*` The callback object
 */
EST_API struct EST_DataCallback *EST_ChannelAddCallback(EST_Channel *handle, EST_DATA_CALLBACK callback, void *userData);

/**
 * Remove callback from the channel
 *
 * @param handle The channel object
 * @param callback The callback object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_ChannelRemoveCallback(EST_Channel *handle, struct EST_DataCallback *callback);

/**
 * Get the data from the channel, the data is based on current position
 *
 * @param channel The channel object
 * @param data The output data
 * @param size The size of the data
 * @param type The type of the data, see `EST_GET_DATA_TYPE` for more information
 *
 * @return `int` The number of data copied
 */
EST_API int EST_ChannelGetData(EST_Channel *channel, float *data, int size, EST_GET_DATA_TYPE type);

#if __cplusplus
}
#endif

#endif