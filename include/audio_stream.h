#pragma once

#include <vector>
#include <optional>
#include <memory>

#include "data_source.h"
#include "minimp3.h"

struct SDL_AudioStream;
struct AudioStreamDeleter
{
    void operator()(SDL_AudioStream*) const;
};

namespace ash
{

using AudioDeviceID = std::uint32_t;

struct DecodingResult
{
    std::vector<int16_t> pcm;
    int sample_rate = 0;
    int channels = 0;
};

class AudioStream
{
public:
    AudioStream() = default;
    AudioStream(std::unique_ptr<DataSource> data_source);
    ~AudioStream();

    AudioStream(AudioStream&&) = default;
    AudioStream& operator=(AudioStream&&) = default;

    AudioStream(const AudioStream&) = delete;
    AudioStream& operator=(const AudioStream&) = delete;

    void play();
    void pause();
    void resume();
    void stop();

private:
    std::optional<DecodingResult> decode_whole_mp3_file();

    mp3dec_t decoder;
    std::unique_ptr<DataSource> data_source;
    std::unique_ptr<SDL_AudioStream, AudioStreamDeleter> stream;
    std::vector<uint8_t> mp3_data;

    AudioDeviceID device = 0;
};

}
