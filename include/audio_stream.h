#pragma once

#include <vector>
#include <array>
#include <memory>
#include <semaphore>

#include "data_source.h"
#include "minimp3.h"
#include "ring_buffer.hpp"

struct SDL_AudioStream;
struct AudioStreamDeleter
{
    void operator()(SDL_AudioStream*) const;
};

constexpr static size_t FILE_BUFFER_SIZE = 64 * 1024;
constexpr static size_t PCM_BUFFER_SIZE = 128 * 1024;

namespace ash
{

using AudioDeviceID = std::uint32_t;

class AudioStream
{
public:
    AudioStream() = delete;
    AudioStream(std::unique_ptr<DataSource> data_source);
    ~AudioStream();

    AudioStream(AudioStream&&) = delete;
    AudioStream& operator=(AudioStream&&) = delete;

    AudioStream(const AudioStream&) = delete;
    AudioStream& operator=(const AudioStream&) = delete;

    void play();
    void pause();
    void resume();
    void stop();

    bool is_playing() const;

private:
    void play_audio_thr();
    void decode_mp3_thr();

    mp3dec_frame_info_t info;
    mp3dec_t decoder;

    std::array<uint8_t, FILE_BUFFER_SIZE> encoded_buffer;
    std::array<int16_t, MINIMP3_MAX_SAMPLES_PER_FRAME> decoded_buffer;
    RingBuffer<int16_t, PCM_BUFFER_SIZE> pcm_buffer;

    std::unique_ptr<DataSource> data_source;
    std::unique_ptr<SDL_AudioStream, AudioStreamDeleter> stream;

    std::array<std::thread, 2> workers;
    std::binary_semaphore first_frame_read {0};

    AudioDeviceID device = 0;

    bool playing = false;
    bool alive = false;
};

}
