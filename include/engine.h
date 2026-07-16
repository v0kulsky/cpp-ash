#pragma once

#include <vector>
#include <optional>
#include <memory>

#include "minimp3.h"
#include "music.h"

struct SDL_AudioStream;
struct AudioStreamDeleter
{
    void operator()(SDL_AudioStream*) const;
};

namespace ash {

using AudioDeviceID = std::uint32_t;

struct DecodingResult
{
    std::vector<int16_t> pcm;
    int sample_rate = 0;
    int channels = 0;
};

class Engine
{
public:
    Engine();
    ~Engine();

    void shutdown();

    void play();
    void pause();
    void resume();

    void load(Music& music);

private:
    std::optional<DecodingResult> decode_whole_mp3_file();

    std::unique_ptr<SDL_AudioStream, AudioStreamDeleter> stream;

    mp3dec_t decoder;
    std::vector<uint8_t> mp3_data;

    Music* current_music = nullptr;
    AudioDeviceID device = 0;
};

std::string get_version();

}
