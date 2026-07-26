#include "engine.h"

namespace ash
{

Engine::Engine()
{
}

Engine::~Engine()
{
}

AudioStream Engine::create_stream(std::unique_ptr<DataSource> data_source)
{
    if (!data_source)
    {
        throw std::invalid_argument("Cannot create stream without data source");
    }

    this->is_stream_created = true;

    return AudioStream(std::move(data_source));
}

std::string get_version()
{
    return "0.1";
}

}
