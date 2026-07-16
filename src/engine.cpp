#include "engine.h"

#include <iostream>
#include <fstream>
#include <SDL3/SDL.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

void AudioStreamDeleter::operator()(SDL_AudioStream* stream) const
{
    SDL_DestroyAudioStream(stream);
}

namespace ash {

Engine::Engine()
{
    mp3dec_init(&this->decoder);

    this->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    if (!this->device)
    {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }
}

Engine::~Engine()
{
    if (this->stream)
    {
        shutdown();
    }
}

void Engine::shutdown()
{
    SDL_ClearAudioStream(this->stream.get());
    this->stream = nullptr;
}

void Engine::play()
{
    if (this->stream)
    {
        SDL_ClearAudioStream(this->stream.get());
        this->stream = nullptr;
    }

    auto result = decode_whole_mp3_file();

    if (!result.has_value())
    {
        return;
    }

    auto decoded_result = result.value();

    SDL_AudioSpec device_spec{};

    if (!SDL_GetAudioDeviceFormat(this->device, &device_spec, nullptr))
    {
        std::cerr << "SDL_GetAudioDeviceFormat failed: " << SDL_GetError()
                  << '\n';
        return;
    }

    SDL_AudioSpec source_spec{};
    source_spec.format = SDL_AUDIO_S16;
    source_spec.freq = decoded_result.sample_rate;
    source_spec.channels = decoded_result.channels;

    this->stream.reset(SDL_CreateAudioStream(&source_spec, &device_spec));

    if (!this->stream)
    {
        std::cerr << "SDL_CreateAudioStream failed: " << SDL_GetError() << '\n';
        return;
    }

    if (!SDL_BindAudioStream(this->device, this->stream.get()))
    {
        std::cerr << "SDL_BindAudioStream failed: " << SDL_GetError() << '\n';
        return;
    }

    if (!SDL_PutAudioStreamData(this->stream.get(),
                                decoded_result.pcm.data(),
                                static_cast<int>(decoded_result.pcm.size() * sizeof(int16_t))))
    {
        std::cerr << "SDL_PutAudioStreamData failed: " << SDL_GetError()
                  << '\n';
        return;
    }

    if (!SDL_ResumeAudioDevice(this->device))
    {
        std::cerr << "SDL_ResumeAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }

    std::cout << "Playing " << this->current_music->get_filepath() << "\n";

    return;
}

void Engine::pause()
{
    if (!SDL_PauseAudioDevice(this->device))
    {
        std::cerr << "SDL_PauseAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }

    return;
}

void Engine::resume()
{
    if (!SDL_ResumeAudioDevice(this->device))
    {
        std::cerr << "SDL_ResumeAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }

    return;
}

void Engine::load(Music& music)
{
    if (this->current_music == &music)
    {
        return;
    }

    if (this->mp3_data.size() != 0)
    {
        this->mp3_data.clear();
    }

    std::ifstream file(music.get_filepath(),
                       std::ios::binary);

    if (!file)
    {
        std::cerr << "Cannot open " << music.get_filepath() << "\n";
        return;
    }

    this->mp3_data.assign(std::istreambuf_iterator<char>(file),
                          std::istreambuf_iterator<char>());

    this->current_music = &music;
}

std::optional<DecodingResult> Engine::decode_whole_mp3_file()
{
    DecodingResult result;

    uint8_t* input = this->mp3_data.data();
    size_t remaining = this->mp3_data.size();

    while (remaining > 0)
    {
        mp3dec_frame_info_t info{};

        int16_t buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];

        int samples = mp3dec_decode_frame(&this->decoder,
                                          input,
                                          remaining,
                                          buffer,
                                          &info);

        if (info.frame_bytes == 0)
            break;

        input += info.frame_bytes;
        remaining -= info.frame_bytes;

        if (samples > 0)
        {
            result.sample_rate = info.hz;
            result.channels = info.channels;

            result.pcm.insert(result.pcm.end(),
                              buffer,
                              buffer + samples * result.channels);
        }
    }

    if (result.pcm.empty())
    {
        std::cerr << "No audio decoded\n";
        return std::nullopt;
    }

    return result;
}

std::string get_version()
{
    return "0.1";
}

}
