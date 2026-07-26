#include <algorithm>
#include <cstring>
#include <iostream>

#include "data_source.h"

namespace ash
{

MemoryDataSource::MemoryDataSource(std::vector<uint8_t> data)
    : file_data(std::move(data))
{}

size_t MemoryDataSource::read(std::span<std::uint8_t> buffer)
{
    const size_t remaining = this->file_data.size() - position;
    const size_t bytes_to_read = std::min(buffer.size(), remaining);

    std::memcpy(buffer.data(), file_data.data() + position, bytes_to_read);

    position += bytes_to_read;

    return bytes_to_read;
}

bool MemoryDataSource::eof() const
{
    return this->position >= this->file_data.size();
}

const size_t MemoryDataSource::size() const
{
    return this->file_data.size();
}

// size_t StreamingDataSource::read(std::span<std::uint8_t> buffer)
// {
//     return 0;
// }

// bool StreamingDataSource::eof() const
// {
//     return false;
// }

// const size_t StreamingDataSource::size() const
// {
//     return 0;
// }

}
