#pragma once

#include <cstdint>
#include <span>
#include <vector>

// #include <ring_bufer.h>

namespace ash
{

class DataSource
{
public:
    virtual ~DataSource() = default;

    virtual size_t read(std::span<std::uint8_t> buffer) = 0;
    virtual bool eof() const = 0;
    virtual const size_t size() const = 0;
};

class MemoryDataSource : public DataSource
{
public:
    MemoryDataSource(std::vector<uint8_t> data);
    ~MemoryDataSource() override = default;

    size_t read(std::span<std::uint8_t> buffer) override;
    const size_t size() const override;
    bool eof() const override;

private:
    std::vector<std::uint8_t> file_data;
    size_t position = 0;
};

// class StreamingDataSource : public DataSource
// {
// public:
//     ~StreamingDataSource() override = default;

//     size_t read(std::span<std::uint8_t> buffer) override;
//     bool eof() const override;
//     const size_t size() const override;

//     void push(std::span<const uint8_t> data);

// private:
//     RingBuffer<std::uint8_t, 512> file_data;
//     bool finished = false;
// };

}
