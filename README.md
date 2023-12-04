# EstAudio - Easy to use audio library

EstAudio is a very simple audio library for any simple playback. It is based on the [miniaudio](https://github.com/mackron/miniaudio/) library and provides a very simple function to play audio files.

EstAudio is also timestretching using [signalsmith-stretch](https://signalsmith-audio.co.uk/code/stretch/)

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
    EST_DeviceInit(44100, 2); // 44.1hz and stereo

    EHANDLE handle;
    EST_RESULT result = EST_SampleLoad(argv[1], &handle);
    if (result != EST_OK) {
        printf("Error loading sample: %s\n", EST_GetError());
        return 1;
    }

    EST_SamplePlay(handle);

    printf("Press enter to exit\n");
    getchar();

    EST_SampleUnload(handle);
    EST_DeviceShutdown();

    return 0;
}
```

Load from memory:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, 2); // 44.1hz and stereo

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

    EST_SampleUnload(handle);
    EST_DeviceShutdown();

    return 0;
}
```

Timestretching:
```c
#include <EstAudio.h>

int main(int argc, char** argv) {
    EST_DeviceInit(44100, 2); // 44.1hz and stereo

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

    EST_SampleUnload(handle);
    EST_DeviceShutdown();

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