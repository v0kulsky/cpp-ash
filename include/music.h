#pragma once

#include <filesystem>
#include <cstdint>
#include <limits>

class SDL_AudioStream;

namespace ash
{

constexpr uint8_t InvalidStreamId = std::numeric_limits<uint8_t>::max();

class Music
{
public:
    Music() = delete;
    Music(std::filesystem::path filepath);

    [[nodiscard]]
    const bool is_loaded() const;

    [[nodiscard]]
    const std::filesystem::path& get_filepath() const;

private:
    std::filesystem::path filepath;
    uint8_t stream_id = InvalidStreamId;
};

}
