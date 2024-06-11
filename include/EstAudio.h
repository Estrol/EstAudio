#ifndef __EST_AUDIO_H
#define __EST_AUDIO_H

#include "EstTypes.h"

#include "Audio/Device.h"
#include "Audio/Sample.h"

#if __cplusplus
extern "C" {
#endif

// Get error message whatever the API function return != ES_OK (NOT THREAD SAFE)
// Returns:
// The error message
// or
// null if no error
EST_API const char *EST_GetError();

// INTERNAL, set error message (NOT THREAD SAFE)
// Params:
// error - The error message
EST_API void EST_SetError(const char *error);

#if __cplusplus
}
#endif

#endif