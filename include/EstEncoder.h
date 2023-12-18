#ifndef __EST_AUDIO_ENCODER_H
#define __EST_AUDIO_ENCODER_H

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

// Load file as encoder channel
EST_API enum EST_RESULT EST_EncoderLoad(const char *path, est_audio_callback callback, enum EST_DECODER_FLAGS flags, EHANDLE *handle);

// Load audio file from memory as encoder channel
EST_API enum EST_RESULT EST_EncoderLoadMemory(const void *data, int size, est_audio_callback callback, enum EST_DECODER_FLAGS flags, EHANDLE *handle);

// Free the encoder channel
EST_API enum EST_RESULT EST_EncoderFree(EHANDLE handle);

// Render the encoder channel
// Note: You don't need use this channel if you using EST_GetSampleChannel
EST_API enum EST_RESULT EST_EncoderRender(EHANDLE handle);

// Set the encoder channel attribute
EST_API enum EST_RESULT EST_EncoderSetAttribute(EHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value);

// Get the encoder channel attribute
EST_API enum EST_RESULT EST_EncoderGetAttribute(EHANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value);

// Get decoded data from encoder channel
// Note: Must use EST_EncoderRender first to get the decoded data
EST_API enum EST_RESULT EST_EncoderGetData(EHANDLE handle, void *data, int *size);

// Flush the encoder channel
EST_API enum EST_RESULT EST_EncoderFlushData(EHANDLE handle);

// Get available decoded PCM data length
// Note: Must use EST_EncoderRender first to get the decoded data
EST_API enum EST_RESULT EST_EncoderGetAvailableDataSize(EHANDLE handle, int *size);

// Convert encoder channel to Sample channel
// Note: No need to use EST_EncoderRender
// Note2: You need free the out sample after you done used it.
EST_API enum EST_RESULT EST_EncoderGetSample(EHANDLE handle, EHANDLE *outSample);

// Export the encoder channel to file
// Note: Need to call EST_EncoderRender first
EST_API enum EST_RESULT EST_EncoderExportFile(EHANDLE handle, enum EST_FILE_EXPORT type, char *filePath);

#if __cplusplus
}
#endif

#endif