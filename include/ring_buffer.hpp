#pragma once

#include <array>
#include <optional>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <cstddef>
#include <iostream>
#include <cstring>

namespace ash
{

template<typename T, std::size_t C> requires (C > 0)
class RingBuffer
{
public:
    RingBuffer();
    RingBuffer(std::initializer_list<T> data);

    void push(const T& item);
    void blocking_push(const T& item);
    std::size_t blocking_write(const T* data, std::size_t count);

    std::optional<T> blocking_pop();
    std::size_t blocking_read(T* container, std::size_t max_count);

    const T& blocking_front() const;
    const T& blocking_back() const;

    T get_by_value(std::size_t index) const;

    void finish();

    void clear();

    bool is_full() const;
    bool is_empty() const;
    bool is_finished() const;

    std::size_t get_size() const;
    static constexpr std::size_t capacity() noexcept;

private:
    mutable std::mutex mtx;
    mutable std::condition_variable non_empty;
    mutable std::condition_variable non_full;

    std::array<T, C> buffer;

    std::size_t read_idx;
    std::size_t write_idx;
    std::size_t size;

    bool finished = false;
};

template<typename T, std::size_t C> requires (C > 0)
RingBuffer<T, C>::RingBuffer()
    : read_idx(0), write_idx(0), size(0)
{}

template<typename T, std::size_t C> requires (C > 0)
RingBuffer<T, C>::RingBuffer(std::initializer_list<T> data)
    : buffer{}
{
    for (const T& value : data)
    {
        push(value);
    }
}

template<typename T, std::size_t C> requires (C > 0)
void RingBuffer<T, C>::push(const T& item)
{
    std::lock_guard<std::mutex> lck{this->mtx};

    if (this->finished)
    {
        return;
    }

    this->buffer[this->write_idx] = item;
    this->write_idx = (this->write_idx + 1) % C;

    if (this->size == C)
    {
        this->read_idx = (this->read_idx + 1) % C;
    }
    else
    {
        ++this->size;
    }

    this->non_empty.notify_one();
}

template<typename T, std::size_t C> requires (C > 0)
void RingBuffer<T, C>::blocking_push(const T& item)
{
    std::unique_lock<std::mutex> lck{this->mtx};
    this->non_full.wait(lck, [this]() {return this->size < C || this->finished;});

    if (this->finished)
    {
        return;
    }

    this->buffer[this->write_idx] = item;
    this->write_idx = (this->write_idx + 1) % C;

    ++this->size;

    this->non_empty.notify_one();
}

template<typename T, std::size_t C> requires (C > 0)
std::size_t RingBuffer<T, C>::blocking_write(const T* data, std::size_t count)
{
    if (data == nullptr || count == 0)
    {
        return 0;
    }

    std::unique_lock<std::mutex> lck{this->mtx};

    std::size_t written = 0;

    while (written < count)
    {
        this->non_full.wait(lck, [this]()
        {
            return this->size < C || this->finished;
        });

        if (this->finished)
        {
            break;
        }

        const std::size_t free_space = C - this->size;

        const std::size_t n = std::min(free_space, count - written);

        const std::size_t first = std::min(n, C - this->write_idx);

        std::memcpy(this->buffer.data() + this->write_idx, data + written,
                    first * sizeof(T));

        this->write_idx = (this->write_idx + first) % C;

        this->size += first;
        written += first;

        if (first < n)
        {
            const std::size_t second = n - first;

            std::memcpy(this->buffer.data() + this->write_idx, data + written,
                        second * sizeof(T));

            this->write_idx = (this->write_idx + second) % C;

            this->size += second;
            written += second;
        }

        this->non_empty.notify_one();
    }

    return written;
}

template<typename T, std::size_t C> requires (C > 0)
std::optional<T> RingBuffer<T, C>::blocking_pop()
{
    std::unique_lock<std::mutex> lck{this->mtx};
    this->non_empty.wait(lck, [this]() {return this->size > 0 || this->finished;});

    if (this->size == 0 && this->finished)
    {
        return std::nullopt;
    }

    const T result{this->buffer[this->read_idx]};
    this->read_idx = (this->read_idx + 1) % C;

    --this->size;

    this->non_full.notify_one();

    return result;
}

template<typename T, std::size_t C> requires (C > 0)
std::size_t RingBuffer<T, C>::blocking_read(T* container, std::size_t max_count)
{
    if (container == nullptr || max_count == 0)
    {
        return 0;
    }

    std::unique_lock<std::mutex> lck{this->mtx};
    this->non_empty.wait(lck, [this]() {return this->size > 0 || this->finished;});

    if (this->size == 0 && this->finished)
    {
        return 0;
    }

    const std::size_t n = std::min(max_count, this->size);

    for (std::size_t i{0}; i < n; ++i)
    {
        container[i] = this->buffer[this->read_idx];
        this->read_idx = (this->read_idx + 1) % C;
    }

    this->size -= n;

    non_full.notify_one();

    return n;
}

template<typename T, std::size_t C> requires (C > 0)
const T& RingBuffer<T, C>::blocking_front() const
{
    std::unique_lock<std::mutex> lck{this->mtx};
    this->non_empty.wait(lck, [this]() {return this->size > 0;});

    return this->buffer[this->read_idx];
}

template<typename T, std::size_t C> requires (C > 0)
const T& RingBuffer<T, C>::blocking_back() const
{
    std::unique_lock<std::mutex> lck{this->mtx};
    this->non_empty.wait(lck, [this]() {return this->size > 0;});

    return this->buffer[(this->read_idx + (this->size - 1)) % C];
}


template<typename T, std::size_t C> requires (C > 0)
T RingBuffer<T, C>::get_by_value(std::size_t index) const
{
   std::lock_guard<std::mutex> lck{this->mtx};

   if (index >= this->size)
   {
       throw std::out_of_range{ "Out of bounds" };
   }

   return this->buffer[(this->read_idx + index) % C];
}

template<typename T, std::size_t C> requires (C > 0)
void RingBuffer<T, C>::clear()
{
   std::lock_guard<std::mutex> lck{this->mtx};

   this->size = this->write_idx = this->read_idx = 0;
   this->finished = false;
}

template<typename T, std::size_t C> requires (C > 0)
void RingBuffer<T, C>::finish()
{
    {
        std::lock_guard<std::mutex> lck{this->mtx};
        this->finished = true;
    }

    non_empty.notify_all();
    non_full.notify_all();
}

template<typename T, std::size_t C> requires (C > 0)
bool RingBuffer<T, C>::is_full() const
{
   std::lock_guard<std::mutex> lck{this->mtx};

   return this->size == C;
}

template<typename T, std::size_t C> requires (C > 0)
bool RingBuffer<T, C>::is_empty() const
{
   std::lock_guard<std::mutex> lck{this->mtx};

   return this->size == 0;
}

template<typename T, std::size_t C> requires (C > 0)
bool RingBuffer<T, C>::is_finished() const
{
   std::lock_guard<std::mutex> lck{this->mtx};

   return this->finished;
}
template<typename T, std::size_t C> requires (C > 0)
std::size_t RingBuffer<T,C>::get_size() const
{
   std::lock_guard<std::mutex> lck{this->mtx};

   return this->size;
}

template<typename T, std::size_t C> requires (C > 0)
constexpr std::size_t RingBuffer<T, C>::capacity() noexcept
{
    return C;
}

}
