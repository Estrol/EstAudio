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

    EST_ATTRIB_VOLUME = 0,     // Volume of the sample
    EST_ATTRIB_SAMPLERATE = 1, // Sample rate of the sample
    EST_ATTRIB_PAN = 2,        // Pan of the sample
    EST_ATTRIB_LOOPING = 3,    // Sample loop

    EST_ATTRIB_FX_TEMPO = 4, // FX tempo control which change audio rate without pitch change (different from sampleRate)
    EST_ATTRIB_FX_PITCH = 5, // FX pitch control without change the audio rate

    EST_ATTRIB_ENCODER_TEMPO = 6,      // Encoder tempo control which change audio rate without pitch change (different from sampleRate)
    EST_ATTRIB_ENCODER_PITCH = 7,      // Encoder pitch control without change the audio rate
    EST_ATTRIB_ENCODER_SAMPLERATE = 8, // Encoder both tempo and pitch control
};

enum EST_STATUS {
    EST_STATUS_UNKNOWN,

    EST_STATUS_IDLE,
    EST_STATUS_PLAYING,
    EST_STATUS_PAUSED,
    EST_STATUS_AT_END
};

enum EST_GET_DATA_TYPE {
    EST_GET_DATA_TYPE_UNKNOWN,
    EST_GET_DATA_TYPE_FFT,        // Get the data as FFT
    EST_GET_DATA_TYPE_INDIVIDUAL, // Get the data as individual channel (not implemented)
    EST_GET_DATA_TYPE_SEEK_TIME,  // Seek the pcm time for the length of the sample
};

#define EST_FFT_4096 8192
#define EST_FFT_1024 2048
#define EST_FFT_512 1024
#define EST_FFT_256 512
#define EST_FFT_128 256

// Export file format, currently only support WAV
// More format coming soon
enum EST_FILE_EXPORT {
    EST_EXPORT_UNKNOWN,
    EST_EXPORT_WAV, // Export sample as wav 16bit format
    EST_EXPORT_OGG
};

typedef struct EST_Channel      EST_Channel;      // The audio channel, used for playing audio file in audio device.
typedef struct EST_FX           EST_FX;           // The audio effect, used for real-time audio processing like pitch, and tempo.
typedef struct EST_Device       EST_Device;       // The audio device, used for audio playback.
typedef struct EST_Sample       EST_Sample;       // The audio sample, used fast audio loading.
typedef struct EST_Encoder      EST_Encoder;      // The audio encoder, used for encoding/processing audio without device.
typedef struct EST_DataCallback EST_DataCallback; // The data callback, used for DSP processing for both device and encoder.

typedef void (*EST_DATA_CALLBACK)(EST_Channel *pHandle, void *pUserData, void *pData, int frameCount);
typedef void (*EST_ENCODER_CALLBACK)(EST_Encoder *pHandle, void *pUserData, void *pData, int frameCount);

typedef struct
{
    int                   sampleRate;  // The sample rate of the device
    int                   channels;    // The number of channels of the device, if the audio channel different from the device channel, the audio will be proccessed to match the device channel
    int                   deviceIndex; // The device index, if the device index is -1, the default device will be used
    enum EST_DEVICE_FLAGS flags;       // The device flags, see `EST_DEVICE_FLAGS` for more information
} est_device_info;

typedef struct
{
    int sampleRate; // The sample rate of the encoder
    int channels;   // The number of channels of the encoder
    int pcmSize;    // The size of the pcm data
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