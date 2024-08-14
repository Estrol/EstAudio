#ifndef __FX_H_
#define __FX_H_

#include "EstTypes.h"

#if __cplusplus
extern "C" {
#endif

/**
 * Create EST_FX from file, this allow using time stretching and pitch shifting
 *
 * @param path The path to the file
 * @return `EST_FX*` The EST_FX object
 */
EST_API struct EST_FX *EST_FXLoad(const char *path);

/**
 * Create EST_FX from file memory, this allow using time stretching and pitch shifting
 *
 * @param data The data of the file
 * @param size The size of the data
 * @return `EST_FX*` The EST_FX object
 */
EST_API struct EST_FX *EST_FXLoadFromMemory(const void *data, size_t size);

/**
 * Free the EST_FX object, this also free underlying the EST_Channels created with `EST_FXCreateChannel`
 * Also free the EST_Sample created with `EST_FXCreateSample`
 *
 * @param fx The `EST_FX` object
 */
EST_API void EST_FXFree(struct EST_FX *fx);

/**
 * Create a EST_Sample from the EST_FX object
 *
 * @param fx The EST_FX object
 * @return EST_Sample* The EST_Sample object
 */
EST_API struct EST_Sample *EST_FXCreateSample(struct EST_FX *fx);

/**
 * Create a EST_Channel from the EST_FX object, this also insert the channel into device.
 *
 * @param fx The EST_FX object
 * @param device The EST_Device object
 * @return EST_Channel* The EST_Channel object
 */
EST_API struct EST_Channel *EST_FXCreateChannel(struct EST_FX *fx, struct EST_Device *device);

/**
 * Set the attribute of the EST_FX object
 *
 * @param fx The EST_FX object
 * @param value See `est_attribute_value`
 * @return EST_RESULT The result of the operation
 */
EST_API enum EST_RESULT EST_FXSetAttribute(struct EST_FX *fx, est_attribute_value *value);

/**
 * Get the attribute of the EST_FX object
 *
 * @param fx The EST_FX object
 * @param value See `est_attribute_value`
 * @return EST_RESULT The result of the operation
 */
EST_API enum EST_RESULT EST_FXGetAttribute(struct EST_FX *fx, est_attribute_value *value);

#if __cplusplus
}
#endif

#endif