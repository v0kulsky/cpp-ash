#include <iostream>
#include <chrono>
#include <cstring>
#include <cassert>
#include <SDL3/SDL.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include "audio_stream.h"

#define SHLOGGER_IMPL
#include "shlogger.hpp"

void AudioStreamDeleter::operator()(SDL_AudioStream* stream) const
{
    SDL_DestroyAudioStream(stream);
}

namespace
{
    constexpr std::size_t SAFE_BUFFER_THRESHOLD = 2048;
    constexpr std::size_t STREAM_QUEUED_BYTES_THRESHOLD = 96000;
    constexpr std::size_t STREAM_FEEDING_SAMPLES = 8192;
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
        LOG_ERROR("AudioStream: SDL_OpenAudioDevice failed: {}", SDL_GetError());
        return;
    }
}

AudioStream::~AudioStream()
{
    stop();
}

void AudioStream::play()
{
    if (this->is_playing)
    {
        return;
    }

    this->is_playing = true;

    this->workers[0] = std::thread(&AudioStream::decode_mp3_thr, this);
    this->workers[1] = std::thread(&AudioStream::play_audio_thr, this);

    return;
}

void AudioStream::pause()
{
    if (!SDL_PauseAudioDevice(this->device))
    {
        LOG_ERROR("AudioStream::pause: Pausing failed: {}", SDL_GetError());
        return;
    }
}

void AudioStream::resume()
{
    if (!SDL_ResumeAudioDevice(this->device))
    {
        LOG_ERROR("AudioStream::resume: Resuming failed: {}", SDL_GetError());
        return;
    }
}

void AudioStream::stop()
{
    this->is_playing = false;

    this->pcm_buffer.finish();

    for (auto& w : this->workers)
    {
        if (w.joinable())
        {
            w.join();
        }
    }

    this->stream.reset();

    SDL_CloseAudioDevice(this->device);
}

void AudioStream::play_audio_thr()
{
    std::array<int16_t, STREAM_FEEDING_SAMPLES> chunk;

    SDL_AudioSpec device_spec{};
    SDL_AudioSpec source_spec{};

    if (!SDL_GetAudioDeviceFormat(this->device, &device_spec, nullptr))
    {
        LOG_ERROR("AudioStream::play_audio_thr: SDL_GetAudioDeviceFormat failed: {}",
                  SDL_GetError());
        return;
    }

    this->first_frame_read.acquire();

    source_spec.format = SDL_AUDIO_S16;
    source_spec.freq = this->info.hz;
    source_spec.channels = this->info.channels;

    this->stream.reset(SDL_CreateAudioStream(&source_spec, &device_spec));

    if (!this->stream)
    {
        LOG_ERROR("AudioStream::play_audio_thr: SDL_CreateAudioStream failed: {}",
                  SDL_GetError());
        return;
    }

    if (!SDL_BindAudioStream(this->device, this->stream.get()))
    {
        LOG_ERROR("AudioStream::play_audio_thr: SDL_BindAudioStream failed: {}",
                  SDL_GetError());
        return;
    }

    while (this->is_playing)
    {
        // check if SDL_AudioStream needs feeding
        if (SDL_GetAudioStreamQueued(this->stream.get()) >= STREAM_QUEUED_BYTES_THRESHOLD)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        LOG_DEBUG("AudioStream::play_audio_thr: "
                  "[1] Audio stream fed with {} bytes.",
                  SDL_GetAudioStreamQueued(this->stream.get()));

        // get data from pcm_buffer into continuous chunk
        std::size_t samples_read =
            this->pcm_buffer.blocking_read(chunk.data() , chunk.size());

        LOG_DEBUG("AudioStream::play_audio_thr: Read {} pcm samples from "
                  "ring buffer (size = {}).",
                  samples_read, this->pcm_buffer.get_size());

        if (samples_read == 0)
        {
            return;
        }

        // feed the chunk to SDL_PutAudioStreamData
        if (!SDL_PutAudioStreamData(this->stream.get(), chunk.data(),
                                    static_cast<int>(samples_read * sizeof(int16_t))))
        {
            LOG_ERROR("AudioStream::play_audio_thr: "
                      "SDL_PutAudioStreamData failed: {}",
                      SDL_GetError());
            return;
        }

        LOG_DEBUG("AudioStream::play_audio_thr: "
                  "[2] Audio stream fed with {} bytes.",
                  SDL_GetAudioStreamQueued(this->stream.get()));
    }
}

void AudioStream::decode_mp3_thr()
{
    int samples;
    bool first_frame_ready = false;
    std::size_t encoded_size = 0;
    std::size_t bytes_read;
    std::size_t remaining;

    while (this->is_playing)
    {
        bytes_read =
            this->data_source->read(std::span(this->encoded_buffer.data() + encoded_size,
                                              this->encoded_buffer.size() - encoded_size));

        LOG_DEBUG("AudioStream::decode_mp3_thr: Read {} bytes from data source.",
                  bytes_read);

        if (bytes_read == 0)
        {
            this->first_frame_read.release();
            this->pcm_buffer.finish();
            break;
        }

        encoded_size += bytes_read;
        remaining = encoded_size;

        uint8_t* input = this->encoded_buffer.data();

        LOG_DEBUG("AudioStream::decode_mp3_thr: Entering decoding inner loop "
                  "with remaining = {} and encoded_size = {}",
                  remaining, encoded_size);

        while (remaining > 0 && this->is_playing)
        {
            // keep remaining buffer bytes in safe range
            if (remaining < SAFE_BUFFER_THRESHOLD)
            {
                break;
            }

            samples = mp3dec_decode_frame(&this->decoder, input, remaining,
                                          this->decoded_buffer.data(), &this->info);

            if (samples <= 0)
            {
                LOG_DEBUG("AudioStream::decode_mp3_thr: Minimp3 decoded 0 or less samples. a frame_bytes {}", this->info.frame_bytes);

                input += this->info.frame_bytes;
                remaining -= this->info.frame_bytes;

                continue;
            }

            input += this->info.frame_bytes;
            remaining -= this->info.frame_bytes;

            LOG_DEBUG("AudioStream::decode_mp3_thr: Going to write {} "
                      "pcm samples to ring buffer (size = {}).",
                      samples * this->info.channels,
                      this->pcm_buffer.get_size());

            this->pcm_buffer.blocking_write(this->decoded_buffer.data(),
                                            samples * this->info.channels);

            if (!first_frame_ready)
            {
                first_frame_ready = true;
                first_frame_read.release();
            }
        }

        if (remaining > 0 && this->is_playing)
        {
            std::memmove(this->encoded_buffer.data(), input, remaining);
        }

        encoded_size = remaining;
    }
}

}
