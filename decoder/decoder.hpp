//
// Created by Darren Otgaar on 2016/01/28.
//

#ifndef SIMPLE_MP3_DECODER_H
#define SIMPLE_MP3_DECODER_H

#include <vector>
#include <algorithm>
#include <iterator>
#ifdef _WIN32
#include <lame.h>
#else
#include <lame/lame.h>
#endif //_WIN32
#include "../base/mp3_stream.hpp"

/*
using byte = unsigned char;

struct mp3_format {
    int samplerate;
    int bitrate;
    int channels;
    int total_frames;
    int duration;   // In seconds
};
*/

template <typename T>
class block_buffer {
public:
    block_buffer() : cursor_(0) { }
    ~block_buffer() { }

    size_t size() const { return buffer_.size(); }

    const T* read_ptr() const { return &(*(buffer_.begin() + cursor_)); }

    void write(T* block, size_t len) {
        std::copy(block, block+len, std::back_inserter(buffer_));
    }

    size_t read(T* block, size_t len) {
        const size_t step = std::min(len, buffer_.size() - cursor_);
        auto start = buffer_.begin() + cursor_;
        auto end = buffer_.begin() + cursor_ + step;
        std::copy(start, end, block);
        cursor_ += step;
        return step;
    }

    size_t skip(size_t len) {
        const size_t step = std::min(len, buffer_.size() - cursor_);
        cursor_ += step;
        return step;
    }

    size_t get_cursor() const { return cursor_; }
    void reset(size_t offset = 0) { cursor_ = offset; }
    bool is_empty() const { return cursor_ == buffer_.size(); }

private:
    size_t cursor_;
    std::vector<T> buffer_;
};

class decoder {
public:
    bool initialise();
    void shutdown();

    block_buffer<short> decode_file(const std::string& filename);

private:
    lame_t lame_;
    hip_t hip_;
    mp3_format format_;
};

#endif //SIMPLE_MP3_DECODER_H
