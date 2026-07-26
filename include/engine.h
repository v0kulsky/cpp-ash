#pragma once

#include "audio_stream.h"

namespace ash {

class Engine
{
public:
    Engine();
    ~Engine();

    AudioStream create_stream(std::unique_ptr<DataSource> data_source);

private:
    bool is_stream_created = false;
};

std::string get_version();

}
