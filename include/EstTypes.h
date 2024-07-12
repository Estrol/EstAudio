#ifndef __EST_TYPES_H
#define __EST_TYPES_H

// clang-format off
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
// clang-format on

#if defined(__cplusplus)
extern "C" {
#endif

enum EST_BOOL {
    EST_FALSE = 0,
    EST_TRUE = 1
};

enum EST_RESULT {
    EST_OK = 0,
    EST_ERROR = 1,

    EST_ERROR_OUT_OF_MEMORY = 2,
    EST_ERROR_INVALID_ARGUMENT = 3,
    EST_ERROR_INVALID_STATE = 4,
    EST_ERROR_INVALID_OPERATION = 5,
    EST_ERROR_INVALID_FORMAT = 6,
    EST_ERROR_INVALID_DATA = 7,
    EST_ERROR_TIMEDOUT = 8,
    EST_ERROR_ENCODER_EMPTY = 9,
    EST_ERROR_ENCODER_UNSUPPORTED = 10,
    EST_ERROR_ENCODER_INVALID_WRITE = 11,
    EST_ERROR_ENCODER_INVALID_READ = 12,
    EST_ERROR_ENCODER_INVALID_OPERATION = 13,
};

enum EST_DEVICE_FLAGS {
    EST_DEVICE_UNKNOWN,

    EST_DEVICE_MONO,   // Single channel audio
    EST_DEVICE_STEREO, // Two channel audio

    EST_DEVICE_FORMAT_S16, // Device signed 16 bit format (NOT IMPLEMENTED)
    EST_DEVICE_FORMAT_F32, // Device 32 bit floating point format (NOT IMPLEMENTED)

    EST_DEVICE_NOSTOP // Prevent the device from stopping when all samples are finished (NOT IMPLEMENTED)
};

enum EST_DECODER_FLAGS {
    EST_DECODER_UNKNOWN,

    EST_DECODER_MONO,
    EST_DECODER_STEREO,

    EST_DECODER_FORMAT_S16, // (NOT IMPLEMENTED)
    EST_DECODER_FORMAT_F32, // (NOT IMPLEMENTED)
};

enum EST_ATTRIBUTE_FLAGS {
    EST_ATTRIB_UNKNOWN,

    EST_ATTRIB_VOLUME = 0,  // Volume of the sample
    EST_ATTRIB_RATE = 1,    // Playback rate of the sample
    EST_ATTRIB_PITCH = 2,   // Pitch toggle
    EST_ATTRIB_PAN = 3,     // Pan of the sample
    EST_ATTRIB_LOOPING = 4, // Sample loop

    EST_ATTRIB_ENCODER_TEMPO = 5,      // Encoder tempo control which change audio rate without pitch change (different from sampleRate)
    EST_ATTRIB_ENCODER_PITCH = 6,      // Encoder pitch control without change the audio rate
    EST_ATTRIB_ENCODER_SAMPLERATE = 7, // Encoder both tempo and pitch control
};

enum EST_STATUS {
    EST_STATUS_UNKNOWN,

    EST_STATUS_IDLE,
    EST_STATUS_PLAYING,
    EST_STATUS_PAUSED,
    EST_STATUS_AT_END
};

enum EST_GET_DATA_TYPE
{
    EST_GET_DATA_TYPE_FFT,
    EST_GET_DATA_TYPE_INDIVIDUAL
};

// Export file format, currently only support WAV
// More format coming soon
enum EST_FILE_EXPORT {
    EST_EXPORT_UNKNOWN,
    EST_EXPORT_WAV, // Export sample as wav 16bit format
    EST_EXPORT_OGG
};

typedef struct EST_Channel EST_Channel;
typedef struct EST_Device EST_Device;
typedef struct EST_Sample EST_Sample;
typedef struct EST_Encoder EST_Encoder;
typedef struct EST_DataCallback EST_DataCallback;

typedef void (*EST_DATA_CALLBACK)(EST_Channel *pHandle, void *pUserData, void *pData, int frameCount);
typedef void (*EST_ENCODER_CALLBACK)(EST_Encoder *pHandle, void *pUserData, void *pData, int frameCount);

typedef struct
{
    int                   sampleRate;
    int                   channels;
    int                   deviceIndex;
    enum EST_DEVICE_FLAGS flags;
} est_device_info;

typedef struct
{
    int sampleRate;
    int channels;
    int pcmSize;
} est_encoder_info;

enum EST_ATTRIB_VAL_TYPE {
    EST_ATTRIB_VAL_FLOAT,
    EST_ATTRIB_VAL_INT,
    EST_ATTRIB_VAL_BOOL
};

enum EST_CHANNEL_POSITION_TYPE {
    EST_CHANNEL_POSITION_PERCENT,
    EST_CHANNEL_POSITION_SAMPLES,
    EST_CHANNEL_POSITION_TIME
};

typedef struct
{
    enum EST_ATTRIBUTE_FLAGS attribute;
    enum EST_ATTRIB_VAL_TYPE type;
    union {
        float    fValue;
        int      iValue;
        EST_BOOL bValue;
    };
} est_attribute_value;

#if defined(__cplusplus)
}
#endif

#endif