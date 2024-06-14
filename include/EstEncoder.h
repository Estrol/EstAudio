#ifndef __EST_AUDIO_ENCODER_H
#define __EST_AUDIO_ENCODER_H

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

// Load file as encoder channel
EST_API struct EST_Encoder *EST_EncoderLoad(const char *path, est_encoder_callback callback, enum EST_DECODER_FLAGS flags);

// Load audio file from memory as encoder channel
EST_API struct EST_Encoder *EST_EncoderLoadMemory(const void *data, int size, est_encoder_callback callback, enum EST_DECODER_FLAGS flags);

// Free the encoder channel
EST_API enum EST_RESULT EST_EncoderFree(struct EST_Encoder *handle);

EST_API enum EST_RESULT EST_EncoderGetInfo(struct EST_Encoder *handle, est_encoder_info *info);

// Render the encoder channel
// Note: You don't need use this channel if you using EST_GetSampleChannel
EST_API enum EST_RESULT EST_EncoderRender(struct EST_Encoder *handle);

// Set the encoder channel attribute
EST_API enum EST_RESULT EST_EncoderSetAttribute(struct EST_Encoder *handle, enum EST_ATTRIBUTE_FLAGS attribute, float value);

// Get the encoder channel attribute
EST_API enum EST_RESULT EST_EncoderGetAttribute(struct EST_Encoder *handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value);

// Get decoded data from encoder channel
// Note: Must use EST_EncoderRender first to get the decoded data
EST_API enum EST_RESULT EST_EncoderGetData(struct EST_Encoder *handle, void *data, int *size);

// Flush the encoder channel
EST_API enum EST_RESULT EST_EncoderFlushData(struct EST_Encoder *handle);

// Get available decoded PCM data length
// Note: Must use EST_EncoderRender first to get the decoded data
EST_API enum EST_RESULT EST_EncoderGetAvailableDataSize(struct EST_Encoder *handle, int *size);

// Export the encoder channel to file
// Note: Need to call EST_EncoderRender first
EST_API enum EST_RESULT EST_EncoderExportFile(struct EST_Encoder *handle, enum EST_FILE_EXPORT type, char *filePath);

#if __cplusplus
}
#endif

#endif