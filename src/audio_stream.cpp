#include <iostream>
#include <SDL3/SDL.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include "audio_stream.h"

void AudioStreamDeleter::operator()(SDL_AudioStream* stream) const
{
    SDL_DestroyAudioStream(stream);
}

namespace ash
{

AudioStream::AudioStream(std::unique_ptr<DataSource> data_source)
    : data_source(std::move(data_source))
{
    mp3dec_init(&this->decoder);

    this->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

    if (!this->device)
    {
        std::cerr << "SDL_OpenAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }
}

AudioStream::~AudioStream()
{
    if (this->stream)
    {
        stop();
    }
}

void AudioStream::play()
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

    return;
}

void AudioStream::pause()
{
    if (!SDL_PauseAudioDevice(this->device))
    {
        std::cerr << "SDL_PauseAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }

    return;
}

void AudioStream::resume()
{
    if (!SDL_ResumeAudioDevice(this->device))
    {
        std::cerr << "SDL_ResumeAudioDevice failed: " << SDL_GetError() << '\n';
        return;
    }

    return;
}

void AudioStream::stop()
{
    SDL_ClearAudioStream(this->stream.get());
    this->stream = nullptr;

    SDL_CloseAudioDevice(this->device);
}

std::optional<DecodingResult> AudioStream::decode_whole_mp3_file()
{
    DecodingResult result;

    const size_t size_of_file = this->data_source->size();
    std::vector<uint8_t> file_data(size_of_file);

    this->data_source->read(file_data);

    uint8_t* input = file_data.data();
    size_t remaining = file_data.size();

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

}
