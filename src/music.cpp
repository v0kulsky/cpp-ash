#include "music.h"

#include <iostream>

namespace ash
{

Music::Music(std::filesystem::path filepath)
    : filepath(std::move(filepath))
{}

const bool Music::is_loaded() const
{
    return true;
}

const std::filesystem::path& Music::get_filepath() const
{
    return this->filepath;
}

}
