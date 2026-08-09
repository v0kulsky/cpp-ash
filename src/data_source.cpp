#include <algorithm>
#include <cstring>
#include <iostream>

#include "data_source.h"
#include "shlogger.hpp"

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

    LOG_DEBUG("MemoryDataSource::read: Read {} bytes of in-memory data.",
              bytes_to_read);

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

StreamingDataSource::StreamingDataSource(const std::filesystem::path& path)
{
    this->file.open(path, std::ios::in | std::ios::binary);
    if (this->file.is_open())
    {
        this->file_size = static_cast<size_t>(std::filesystem::file_size(path));
    }
    else
    {
        LOG_ERROR("StreamingDataSrouce: File {} cannot be opened.", path.string());
    }
}

size_t StreamingDataSource::read(std::span<std::uint8_t> buffer)
{
    if (!this->file.is_open() || this->file.eof())
    {
        return 0;
    }

    this->file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());

    const size_t bytes_read = static_cast<size_t>(this->file.gcount());

    LOG_DEBUG("StreamingDataSource::read: Read {} bytes of streamed data.",
              bytes_read);

    return bytes_read;
}

bool StreamingDataSource::eof() const
{
    return this->file.eof();
}

const size_t StreamingDataSource::size() const
{
    return this->file_size;
}

bool StreamingDataSource::is_ok() const
{
    return !this->file.fail();
}

}
