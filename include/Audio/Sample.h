#ifndef _SAMPLE_H_
#define _SAMPLE_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

// Get the sample rate of the audio system
// Params:
// path - The path to the audio file
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was loaded successfully
// EST_OUT_OF_MEMORY - The sample failed to load due to lack of memory
// EST_INVALID_ARGUMENT - The sample failed to load due to invalid arguments
// EST_INVALID_STATE - The sample failed to load due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleLoad(EST_DEVICE_HANDLE device_handle, const char *path, EST_AUDIO_HANDLE *handle);

// Get the sample rate of the audio system
// Params:
// data - The data of the audio file
// size - The size of the audio file
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was loaded successfully
// EST_OUT_OF_MEMORY - The sample failed to load due to lack of memory
// EST_INVALID_ARGUMENT - The sample failed to load due to invalid arguments
// EST_INVALID_STATE - The sample failed to load due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleLoadMemory(EST_DEVICE_HANDLE device_handle, const void *data, int size, EST_AUDIO_HANDLE *handle);

// Load a raw PCM audio sample
// Note: Must be in format 32-bit float, 2 channel interleaved
// Params:
// data - The data of the audio file
// pcmSize - The size of raw audio in pcm size
// channels - The number of channels of the audio file
// sampleRate - The sample rate of the audio file
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was loaded successfully
// EST_OUT_OF_MEMORY - The sample failed to load due to lack of memory
// EST_INVALID_ARGUMENT - The sample failed to load due to invalid arguments
// EST_INVALID_STATE - The sample failed to load due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleLoadRawPCM(EST_DEVICE_HANDLE device_handle, const void *data, int pcmSize, int channels, int sampleRate, EST_AUDIO_HANDLE *handle);

// Unload the audio sample
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was unloaded successfully
// EST_INVALID_ARGUMENT - The sample failed to unload due to invalid arguments
// EST_INVALID_STATE - The sample failed to unload due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleFree(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle);

// Play the audio sample
// This also resets the sample to the beginning
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was played successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SamplePlay(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle);

// Stop the audio sample
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was played successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleStop(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle);

// Get the status of the audio sample
// Params:
// handle - The handle to the audio sample
// value - The status of the audio sample [see EST_STATUS]
// Returns:
// EST_OK - The sample status was retrieved successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleGetStatus(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, enum EST_STATUS *value);

// Set the attribute of the audio sample
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleSetAttribute(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value);

// Get the attribute of the audio sample
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleGetAttribute(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float *value);

// Slide the attribute over the time
// Note: This is thread blocking function
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// time - The time to how long the attribute slide it, 0 meaning instant
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleSlideAttribute(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value, float time);

// Slide the attribute over the time
// Note: This is async version (aka non-blocking) function
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// time - The time to how long the attribute slide it, 0 meaning instant
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleSlideAttributeAsync(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, enum EST_ATTRIBUTE_FLAGS attribute, float value, float time);

EST_API enum EST_RESULT EST_SampleSetCallback(EST_DEVICE_HANDLE device_handle, EST_AUDIO_HANDLE handle, est_audio_callback callback, void *userdata);
EST_API enum EST_RESULT EST_SampleSetGlobalCallback(EST_DEVICE_HANDLE device_handle, est_audio_callback callback, void *userdata);

#if __cplusplus
}
#endif

#endif