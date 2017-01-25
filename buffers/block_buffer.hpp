//
// Created by Darren Otgaar on 2017/01/16.
//

#ifndef ZAPAUDIO_BLOCK_BUFFER_HPP
#define ZAPAUDIO_BLOCK_BUFFER_HPP

#include <vector>

template <typename T>
class block_buffer {
public:
    block_buffer() : cursor_(0) { }
    ~block_buffer() = default;

    size_t size() const { return buffer_.size(); }

    const T* read_ptr() const { return &(*(buffer_.begin() + cursor_)); }

    void write(T* block, size_t len) {
        std::copy(block, block+len, std::back_inserter(buffer_));
    }

    size_t read(T* block, size_t len) {
        const size_t step = std::min(len, buffer_.size() - cursor_);
        std::copy(buffer_.begin() + cursor_, buffer_.begin() + cursor_ + step, block);
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

#endif //ZAPAUDIO_BLOCK_BUFFER_HPP
