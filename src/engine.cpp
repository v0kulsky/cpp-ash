#include "engine.h"

#include <iostream>
#include <vector>
#include <fstream>
#include <SDL3/SDL.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

namespace ash {

const std::string get_version()
{
    return "0.1";
}

void Engine::play(const Music& music)
{
        // --------------------------------
    // Load MP3 file
    // --------------------------------

    std::ifstream file(
        music.get_filepath(),
        std::ios::binary
    );


    if (!file)
    {
        std::cerr
            << "Cannot open " << music.get_filepath() << "\n";

        return;
    }


    std::vector<uint8_t> mp3_data(
        std::istreambuf_iterator<char>(file),
        {}
    );


    std::cout
        << "MP3 size: "
        << mp3_data.size()
        << " bytes\n";



    // --------------------------------
    // Decode MP3
    // --------------------------------

    mp3dec_t decoder;

    mp3dec_init(&decoder);


    std::vector<int16_t> pcm;


    uint8_t* input = mp3_data.data();
    size_t remaining = mp3_data.size();


    int sample_rate = 0;
    int channels = 0;


    while (remaining > 0)
    {
        mp3dec_frame_info_t info{};


        int16_t buffer[
            MINIMP3_MAX_SAMPLES_PER_FRAME
        ];


        int samples =
            mp3dec_decode_frame(
                &decoder,
                input,
                remaining,
                buffer,
                &info
            );


        if (info.frame_bytes == 0)
            break;


        input += info.frame_bytes;
        remaining -= info.frame_bytes;


        if (samples > 0)
        {
            sample_rate = info.hz;
            channels = info.channels;


            pcm.insert(
                pcm.end(),
                buffer,
                buffer + samples * channels
            );
        }
    }


    if (pcm.empty())
    {
        std::cerr
            << "No audio decoded\n";

        return;
    }


    std::cout
        << "Decoded!\n"
        << "Frequency: "
        << sample_rate
        << "\nChannels: "
        << channels
        << "\nSamples: "
        << pcm.size()
        << '\n';



    // --------------------------------
    // Open SDL audio device
    // --------------------------------

    SDL_AudioDeviceID device =
        SDL_OpenAudioDevice(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
            nullptr
        );


    if (!device)
    {
        std::cerr
            << "SDL_OpenAudioDevice failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    SDL_AudioSpec device_spec{};


    if (!SDL_GetAudioDeviceFormat(
            device,
            &device_spec,
            nullptr))
    {
        std::cerr
            << "SDL_GetAudioDeviceFormat failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    std::cout
        << "Device:\n"
        << "Frequency: "
        << device_spec.freq
        << "\nChannels: "
        << static_cast<int>(device_spec.channels)
        << '\n';



    // --------------------------------
    // Create audio stream
    // --------------------------------

    SDL_AudioSpec source_spec{};

    source_spec.format =
        SDL_AUDIO_S16;

    source_spec.freq =
        sample_rate;

    source_spec.channels =
        channels;



    SDL_AudioStream* stream =
        SDL_CreateAudioStream(
            &source_spec,
            &device_spec
        );


    if (!stream)
    {
        std::cerr
            << "SDL_CreateAudioStream failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    if (!SDL_BindAudioStream(
            device,
            stream))
    {
        std::cerr
            << "SDL_BindAudioStream failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    // --------------------------------
    // Send PCM data
    // --------------------------------

    if (!SDL_PutAudioStreamData(
            stream,
            pcm.data(),
            static_cast<int>(
                pcm.size() * sizeof(int16_t))))
    {
        std::cerr
            << "SDL_PutAudioStreamData failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    // --------------------------------
    // Start playback
    // --------------------------------

    if (!SDL_ResumeAudioDevice(device))
    {
        std::cerr
            << "SDL_ResumeAudioDevice failed: "
            << SDL_GetError()
            << '\n';

        return;
    }



    std::cout
        << "Playing...\n";


    // -------------
    this->current_state = State::playing;

    return;
}

void Engine::pause()
{
    this->current_state = State::paused;

    return;
}

void Engine::resume()
{
    return;
}

void Engine::load(Music& music)
{
    auto tmp = music;

    std::cout << "Given filepath for music: " << music.get_filepath()
              << std::endl;
}

}
