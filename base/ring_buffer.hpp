//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef SIMPLE_MP3_RING_BUFFER_HPP
#define SIMPLE_MP3_RING_BUFFER_HPP

#include <vector>
#include <numeric>

// Note: This is an overwriting ring buffer.  If the buffer is full, it will start overwriting the oldest data.
// The implementation may optionally check the size of the buffer prior to writing to implement queueing; however, for
// audio, it usually makes sense to just overwrite.

template <typename T>
class ring_buffer {
public:
    static_assert(std::is_trivially_constructible<T>::value && std::is_copy_constructible<T>::value,
                  "ring_buffer<> type requires trivial construction and copy construction");
    ring_buffer(size_t mod) : mod_(mod), read_(0), write_(0), buffer_(mod) { }
    ~ring_buffer() = default;

    void clear() { read_ = write_ = 0; }
    bool is_empty() const { return read_ == write_; }
    bool is_full() const { return read_ == (write_ + 1) % mod_; }
    size_t read_position() const { return read_; }
    const T* read_ptr() const { return buffer_.data()+read_; }
    const T* write_ptr() const { return buffer_.data()+write_; }
    size_t write_position() const { return write_; }
    size_t capacity() const { return mod_; }
    size_t size() const { return read_ <= write_ ? write_ - read_ : mod_ - read_ + write_ + 1; }
    size_t remaining() const { return read_ <= write_ ? mod_ - write_ + read_ : read_ - write_ - 1; }

    size_t write(const T* ptr, size_t len) {
        if(write_ + len < read_) {
            std::copy(ptr, ptr+len, buffer_.begin()+write_);
            write_ += len;
        } else if(write_ + len >= read_) {
            if(write_ + len >= buffer_.size()) {
                size_t d = buffer_.size() - write_;
                std::copy(ptr, ptr+d, buffer_.begin()+write_);
                if(d != mod_) std::copy(ptr+d, ptr+len, buffer_.begin());
                if((write_ + len) % mod_ >= read_) read_ = (write_ + len) % mod_ + 1;
            } else {
                std::copy(ptr, ptr+len, buffer_.begin()+write_);
            }
            write_ = (write_ + len) % mod_;
        }
        return len;
    }

    size_t read(T* ptr, size_t len) {
        if(read_ + len <= write_) {
            std::copy(buffer_.begin() + read_, buffer_.begin() + read_ + len, ptr);
            read_ += len;
            return len;
        } else if(read_ < write_) {
            std::copy(buffer_.begin() + read_, buffer_.begin() + write_, ptr);
            read_ = write_;
            return write_ - read_;
        } else if(read_ > write_) {
            if(read_ + len < buffer_.size()) {
                std::copy(buffer_.begin()+read_, buffer_.begin() + read_ + len, ptr);
                read_ += len;
                return len;
            } else {
                size_t d = buffer_.size() - read_;
                std::copy(buffer_.begin()+read_, buffer_.begin()+read_+d, ptr);
                if(len - d <= write_) {
                    std::copy(buffer_.begin(), buffer_.begin()+(len-d), ptr+d);
                    read_ = len - d;
                    return len;
                } else {
                    std::copy(buffer_.begin(), buffer_.begin()+write_, ptr+d);
                    read_ = write_;
                    return d + write_;
                }
            }
        } else {
            return 0;
        }
    }

    size_t peek(T* ptr, size_t len) {
        if(read_ + len <= write_) {
            std::copy(buffer_.begin() + read_, buffer_.begin() + read_ + len, ptr);
            return len;
        } else if(read_ < write_) {
            std::copy(buffer_.begin() + read_, buffer_.begin() + write_, ptr);
            read_ = write_;
            return write_ - read_;
        } else if(read_ > write_) {
            if(read_ + len < buffer_.size()) {
                std::copy(buffer_.begin()+read_, buffer_.begin() + read_ + len, ptr);
                return len;
            } else {
                size_t d = buffer_.size() - read_;
                std::copy(buffer_.begin()+read_, buffer_.begin()+read_+d, ptr);
                if(len - d <= write_) {
                    std::copy(buffer_.begin(), buffer_.begin()+(len-d), ptr+d);
                    return len;
                } else {
                    std::copy(buffer_.begin(), buffer_.begin()+write_, ptr+d);
                    return d + write_;
                }
            }
        } else {
            return 0;
        }
    }

    size_t skip(size_t len) {
        if(read_ + len <= write_) {
            read_ += len;
            return len;
        } else if(read_ < write_) {
            read_ = write_;
            return write_ - read_;
        } else if(read_ > write_) {
            if(read_ + len < buffer_.size()) {
                read_ += len;
                return len;
            } else {
                size_t d = buffer_.size() - read_;
                if(len - d <= write_) {
                    read_ = len - d;
                    return len;
                } else {
                    read_ = write_;
                    return d + write_;
                }
            }
        } else {
            return 0;
        }
    }

private:
    const size_t mod_;
    size_t read_;
    size_t write_;
    std::vector<T> buffer_;
};

#endif //SIMPLE_MP3_RING_BUFFER_HPP
