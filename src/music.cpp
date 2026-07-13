#include "music.h"

#include <iostream>

namespace ash
{

Music::Music(std::filesystem::path filepath)
    : filepath(std::move(filepath))
{
    std::cout << "Created Music instance with stream_id = "
              << static_cast<int>(this->stream_id)
              << std::endl;
}

const bool Music::is_loaded() const
{
    return true;
}

const std::filesystem::path& Music::get_filepath() const
{
    return this->filepath;
}

}
