#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

/**
 * Create a new audio device
 *
 * @param sampleRate The sample rate of the device
 * @param flags The flags of the device, see `EST_DEVICE_FLAGS`
 *
 * @return `EST_Device*` The device object
 */
EST_API struct EST_Device *EST_DeviceInit(int sampleRate, enum EST_DEVICE_FLAGS flags);

/**
 * Get the device information
 *
 * @param device The device object
 * @param info The device information, see `est_device_info`
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_GetInfo(EST_Device *device, est_device_info *info);

/**
 * Add a new callback to the device, used device-level for DSP processing
 *
 * @param device The device object
 * @param callback The callback function
 * @param userData The user data
 *
 * @return `EST_DataCallback*` The callback object
 */
EST_API struct EST_DataCallback *EST_DeviceAddCallback(EST_Device *device, EST_DATA_CALLBACK callback, void *userData);

/**
 * Remove callback from the device
 *
 * @param device The device object
 * @param callback The callback object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_DeviceRemoveCallback(EST_Device *device, struct EST_DataCallback *callback);

/**
 * Shutdown the device, this also free the device
 *
 * @param device The device object
 *
 * @return `EST_RESULT` The result of the operation
 */
EST_API enum EST_RESULT EST_DeviceFree(EST_Device *device);

#if __cplusplus
}
#endif

#endif