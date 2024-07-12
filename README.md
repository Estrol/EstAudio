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

EstAudio Encoder support following formats for export audio:
- WAV
- OGG

# Building
To build EstAudio, you need to have [vcpkg](https://github.com/microsoft/vcpkg) installed as well as [CMake](https://cmake.org/).

Run the following commands to build EstAudio:
```bash
git clone https://github.com/estrol/EstAudio.git
cd EstAudio
cmake --preset=x64-windows-debug
cmake --build build
```

or use this command to build Release build
```
cmake --preset=x64-windows
cmake --build build
```

# Bindings
EstAudio has it's C export functions for use in other languages. \
The bindings currently only available for C# in [Estrol.Audio](https://github.com/Estrol/Estrol.Audio).

# License
EstAudio is licensed under the MIT license. See [LICENSE](LICENSE) for more information.