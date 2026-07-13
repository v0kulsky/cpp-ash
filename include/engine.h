#pragma once

#include <string>

#include "music.h"

namespace ash {

enum class State
{
    playing,
    paused
};

const std::string get_version();

class Engine
{
public:
    void play(const Music& music);
    void pause();
    void resume();

    void load(Music& music);

private:
    Music* current_music = nullptr;
    State current_state = State::paused;
};

}
