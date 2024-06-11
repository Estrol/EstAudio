#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

// Initialize the audio system
// Params:
// sampleRate - The sample rate of the audio system
// flags - The device flags
// Returns:
// EST_OK - The audio system was initialized successfully
// EST_OUT_OF_MEMORY - The audio system failed to initialize due to lack of memory
// EST_INVALID_ARGUMENT - The audio system failed to initialize due to invalid arguments
// EST_INVALID_STATE - The audio system failed to initialize due to invalid state (Already initialized)
// EST_INVALID_OPERATION - The audio system failed to initialize due to backend issues
EST_API enum EST_RESULT EST_DeviceInit(int sampleRate, enum EST_DEVICE_FLAGS flags, EST_DEVICE_HANDLE *out);

EST_API enum EST_RESULT EST_GetInfo(EST_DEVICE_HANDLE device_handle, est_device_info *info);

// Shutdown the audio system
// Returns:
// EST_OK - The audio system was shutdown successfully
// EST_INVALID_STATE - The audio system failed to shutdown due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_DeviceFree(EST_DEVICE_HANDLE device_handle);

#if __cplusplus
}
#endif

#endif