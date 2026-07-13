#include "engine.h"

#include <iostream>

namespace ash {

const std::string get_version()
{
    return "0.1";
}

void Engine::play(const Music& music)
{
    const Music& tmp = music;

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
