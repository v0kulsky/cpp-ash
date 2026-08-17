# Ash Audio Engine

Audio engine built from scratch in C++ using SDL3 and minimp3. It is currently in a prototype / early-stage form, but it provides audio streams backed by different data sources.
Audio data can either be loaded completely into memory or streamed from a file without loading the entire track at once. \
An example application using this audio engine is [Ash Music Player](https://www.github.com/v0kulsky/cpp-ash-player.git). Feel free to look at that project too.

## Overview

The engine separates audio data access from audio playback through two main concepts:

- `DataSource` — provides audio data to the engine.
- `AudioStream` — handles playback of audio data provided by a data source.

Currently available data sources are:

- `MemoryDataSource` — plays audio data that has been completely loaded into memory.
- `StreamingDataSource` — streams audio data directly from a file without loading the entire track into memory.

## Example of usage
### Playing sound completely loaded into memory
```cpp
// 'data' contains the entire audio file loaded into memory (bytes data).
ash::MemoryDataSource mem_ds(std::move(data));

ash::AudioStream stream(mem_ds);
stream.play();
```
### Playing music track as streaming data source
```cpp
// 'path' points to an audio file that will be streamed (std::filesystem::path).
ash::StreamingDataSource streaming_ds(path);

ash::AudioStream stream(streaming_ds);
stream.play();
```

## Dependencies

### SDL3

The project requires [SDL3](https://wiki.libsdl.org/SDL3/FrontPage) to be installed on the system.
CMake will look for an existing SDL3 installation when configuring the project.

### minimp3

[minimp3](https://github.com/lieff/minimp3) is already included in the repository, so no separate installation is required.

## Building the project
### Requirements
- C++ compiler with C++20 support
- CMake 3.28 or newer

### Procedure
This is standard CMake project so to build it you need to:
1. Create `build` directory.
2. Run `cmake -S <path_to_repository> -B <path_to_build_dir>`.
3. Run `cmake --build <path_to_build_dir>`.

After that, the library should be built and ready to be linked to your projects.

## Supported formats

Currently, only MP3 audio files are supported.

## Planned features

- A more unified way of creating and managing audio streams through an `Engine` class.
- Support for additional audio formats.
- DSP and more general audio stream processing and modulation.
- Spatial audio support, including positioning and spatialization of audio streams.
