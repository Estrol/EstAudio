#ifndef __EST_AUDIO_H
#define __EST_AUDIO_H

// clang-format off
#ifndef __API_H_
	#if defined(EST_EXPORT)
		#if defined(_WIN32)
			#define EST_API __declspec(dllexport)
		#elif defined(__GNUC__)
			#define EST_API __attribute__((visibility("default")))
		#endif
	#else
		#if defined(_WIN32)
			#define EST_API __declspec(dllimport)
		#elif defined(__GNUC__)
			#define EST_API
		#endif
	#endif
#endif
// clang-format on

#if __cplusplus
extern "C" {
#endif

enum EST_BOOL {
    EST_FALSE = 0,
    EST_TRUE = 1
};

enum EST_RESULT {
    EST_OK = 0,
    EST_ERROR = 1,

    EST_OUT_OF_MEMORY = EST_ERROR | 2,
    EST_INVALID_ARGUMENT = EST_ERROR | 3,
    EST_INVALID_STATE = EST_ERROR | 4,
    EST_INVALID_OPERATION = EST_ERROR | 5,
    EST_INVALID_FORMAT = EST_ERROR | 6,
    EST_INVALID_DATA = EST_ERROR | 7,
    EST_TIMEDOUT = EST_ERROR | 8,
};

enum EST_ATTRIBUTE {
    EST_ATTRIB_VOLUME = 0,
    EST_ATTRIB_RATE = 1,
    EST_ATTRIB_PITCH = 2,
    EST_ATTRIB_PAN = 3,
    EST_ATTRIB_LOOPING = 4,
};

enum EST_STATUS {
    EST_STATUS_IDLE,
    EST_STATUS_PLAYING,
    EST_STATUS_AT_END
};

typedef unsigned int EHANDLE;
typedef unsigned int EUINT32;
#define INVALID_HANDLE -1

typedef void (*est_audio_callback)(EHANDLE pHandle, void *pUserData, void *pData, int frameCount);

// Initialize the audio system
// Params:
// sampleRate - The sample rate of the audio system
// channels - The number of channels of the audio system
// Returns:
// EST_OK - The audio system was initialized successfully
// EST_OUT_OF_MEMORY - The audio system failed to initialize due to lack of memory
// EST_INVALID_ARGUMENT - The audio system failed to initialize due to invalid arguments
// EST_INVALID_STATE - The audio system failed to initialize due to invalid state (Already initialized)
// EST_INVALID_OPERATION - The audio system failed to initialize due to backend issues
EST_API enum EST_RESULT EST_DeviceInit(int sampleRate, int channels);

// Shutdown the audio system
// Returns:
// EST_OK - The audio system was shutdown successfully
// EST_INVALID_STATE - The audio system failed to shutdown due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_DeviceShutdown();

// Get the sample rate of the audio system
// Params:
// path - The path to the audio file
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was loaded successfully
// EST_OUT_OF_MEMORY - The sample failed to load due to lack of memory
// EST_INVALID_ARGUMENT - The sample failed to load due to invalid arguments
// EST_INVALID_STATE - The sample failed to load due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleLoad(const char *path, EHANDLE *handle);

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
EST_API enum EST_RESULT EST_SampleLoadMemory(const void *data, int size, EHANDLE *handle);

// Unload the audio sample
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was unloaded successfully
// EST_INVALID_ARGUMENT - The sample failed to unload due to invalid arguments
// EST_INVALID_STATE - The sample failed to unload due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleUnload(EHANDLE handle);

// Play the audio sample
// This also resets the sample to the beginning
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was played successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SamplePlay(EHANDLE handle);

// Stop the audio sample
// Params:
// handle - The handle to the audio sample
// Returns:
// EST_OK - The sample was played successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleStop(EHANDLE handle);

// Get the status of the audio sample
// Params:
// handle - The handle to the audio sample
// value - The status of the audio sample [see EST_STATUS]
// Returns:
// EST_OK - The sample status was retrieved successfully
// EST_INVALID_ARGUMENT - The sample failed to play due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleGetStatus(EHANDLE handle, enum EST_STATUS *value);

// Set the attribute of the audio sample
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleSetAttribute(EHANDLE handle, enum EST_ATTRIBUTE attribute, float value);

// Get the attribute of the audio sample
// Params:
// handle - The handle to the audio sample
// attribute - The attribute to set [see EST_ATTRIBUTE]
// value - The value to set the attribute to
// Returns:
// EST_OK - The sample attribute was set successfully
// EST_INVALID_ARGUMENT - The sample failed to set the attribute due to invalid arguments
// EST_INVALID_STATE - The sample failed to play due to invalid state (Not initialized)
EST_API enum EST_RESULT EST_SampleGetAttribute(EHANDLE handle, enum EST_ATTRIBUTE attribute, float *value);

EST_API enum EST_RESULT EST_SampleSetCallback(EHANDLE handle, est_audio_callback callback, void *userdata);
EST_API enum EST_RESULT EST_SampleSetGlobalCallback(est_audio_callback callback, void *userdata);

// Get error message whatever the API function return != ES_OK
// Returns:
// The error message
// or
// null if no error
EST_API const char *EST_GetError();

// INTERNAL, set error message
// Params:
// error - The error message
EST_API void EST_SetError(const char *error);

#if __cplusplus
}
#endif

#endif