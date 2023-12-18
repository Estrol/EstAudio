# EstAudio - Easy to use audio library

EstAudio is a very simple audio library for any simple playback. It is based on the [miniaudio](https://github.com/mackron/miniaudio/) library and provides a very simple function to play audio files.

EstAudio is also timestretching using [signalsmith-stretch](https://signalsmith-audio.co.uk/code/stretch/)

# Features
EstAudio has following features:
1. Very simple API
2. Support for WAV, MP3, OGG, FLAC
3. Real-time timestretching, pitchshifting and resampling
4. Encoder for timestretching, pitchshifting, resampler and exporting to file

# Supported formats
EstAudio supports the following formats:
- WAV
- MP3
- OGG
- FLAC

# Usage
Basic audio playback:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, EST_DEVICE_STEREO); // 44.1hz and stereo

    EHANDLE handle;
    EST_RESULT result = EST_SampleLoad(argv[1], &handle);
    if (result != EST_OK) {
        printf("Error loading sample: %s\n", EST_GetError());
        return 1;
    }

    EST_SamplePlay(handle);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleFree(handle);
    EST_DeviceFree();

    return 0;
}
```

Load from memory:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, EST_DEVICE_STEREO); // 44.1hz and stereo

    // Load the sample from memory
    const char* data = /* ... */;
    size_t size = /* ... */;

    EHANDLE handle;
    EST_RESULT result = EST_SampleLoadMemory(data, size, &handle);
    if (result != EST_OK) {
        printf("Error loading sample: %s\n", EST_GetError());
        return 1;
    }

    EST_SamplePlay(handle);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleFree(handle);
    EST_DeviceFree();

    return 0;
}
```

Timestretching:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, EST_DEVICE_STEREO); // 44.1hz and stereo

    EHANDLE handle;
    EST_RESULT result = EST_SampleLoad(argv[1], &handle);
    if (result != EST_OK) {
        printf("Error loading sample: %s\n", EST_GetError());
        return 1;
    }

    // Set the rate to 1.5x and disable pitch
    EST_SampleSetAttribute(handle, EST_ATTRIB_RATE, 1.5f);
    EST_SampleSetAttribute(handle, EST_ATTRIB_PITCH, EST_FALSE); // or EST_TRUE if you want use pitch

    EST_SamplePlay(handle);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleFree(handle);
    EST_DeviceFree();

    return 0;
}
```

Encoder, and export to file:
```c
#include <EstEncoder.h>

int main(int argc, char** argv) {
    EHANDLE handle;
    EST_RESULT result = EST_EncoderLoad(argv[1], NULL, EST_DECODER_UNKNOWN, &handle);
    if (result != EST_OK) {
        printf("Error loading encoder: %s\n", EST_GetError());
        return 1;
    }

    // Timestretch by 1.5x
    EST_EncoderSetAttribute(handle, EST_ATTRIB_ENCODER_TEMPO, 1.5f);
    EST_EncoderSetAttribute(handle, EST_ATTRIB_PAN, 0.2f);

    EST_EncoderRender(handle);
    EST_EncoderExportFile(handle, EST_EXPORT_WAV, "output.wav");

    EST_EncoderFree(handle);
    return 0;
}
```

Encoder, and play it:
```c
#include <EstAudio.h>
#include <EstEncoder.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, EST_DEVICE_STEREO); // 44.1hz and stereo

    EHANDLE handle;
    EST_RESULT result = EST_EncoderLoad(argv[1], NULL, EST_DECODER_UNKNOWN, &handle);
    if (result != EST_OK) {
        printf("Error loading encoder: %s\n", EST_GetError());
        return 1;
    }
    
    // Timestretch by 1.5x
    EST_EncoderSetAttribute(handle, EST_ATTRIB_ENCODER_TEMPO, 1.5f);
    EST_EncoderSetAttribute(handle, EST_ATTRIB_PAN, 0.2f);

    EHANDLE sample;
    EST_EncoderGetSample(handle, &sample);

    EST_SamplePlay(sample);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleFree(sample);
    EST_EncoderFree(handle);
    return 0;
}
```

Read raw PCM and play it:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, EST_DEVICE_STEREO); // 44.1hz and stereo

    float* data = /* ... */;
    size_t pcmSize = /* ... */;

    EHANDLE handle;
    EST_RESULT result = EST_SampleLoadRawPCM(data, pcmSize, 2, 44100, &handle);
    if (result != EST_OK) {
        printf("Error loading encoder: %s\n", EST_GetError());
        return 1;
    }

    EST_SamplePlay(handle);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleFree(sample);
    EST_DeviceFree();
    return 0;
}
```

# Building
To build EstAudio, you need to have [vcpkg](https://github.com/microsoft/vcpkg) installed as well as [CMake](https://cmake.org/).

Run the following commands to build EstAudio:
```bash
git clone https://github.com/estrol/EstAudio.git
cd EstAudio
cmake --preset=x64-windows
cmake --build build --config Release
```

# License
EstAudio is licensed under the MIT license. See [LICENSE](LICENSE) for more information.