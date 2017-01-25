//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef ZAPAUDIO_RING_BUFFER_HPP
#define ZAPAUDIO_RING_BUFFER_HPP

#include <vector>
#include <numeric>
#include <cassert>

template <typename T, typename IndexT, typename CursorT=std::atomic<IndexT>>
class ring_buffer {
public:
    static_assert(std::is_integral<IndexT>::value, "IndexT must be integral type");

    using type = T;
    using ptr_t = T*;
    using idx_type = IndexT;
    using cursor_type = CursorT;

    ring_buffer() = default;
    ring_buffer(size_t size) : buffer_(size+1), read_(0), write_(0), mod_(idx_type(size+1)) {
        assert(size <= std::numeric_limits<int32_t>::max() && "size must be < max(int32_t)-1");
    }
    ring_buffer(const ring_buffer& rhs) = delete;
    ~ring_buffer() = default;

    ring_buffer& operator=(const ring_buffer& rhs) = delete;

    // These are not atomic!
    void clear() { read_ = write_ = 0; }
    void resize(size_t size) {
        clear();
        buffer_.resize(size+1);
        mod_ = idx_type(size+1);
    }

    idx_type read_cursor() const { return read_; }
    idx_type write_cursor() const { return write_; }
    const ptr_t read_ptr() {
        idx_type curr_read = read_;
        if(curr_read != write_) return &buffer_[curr_read];
        else                    return nullptr;
    }
    idx_type modulus() const { return mod_; }

    bool empty() const { return size() == 0; }
    idx_type size() const {
        idx_type curr_read = read_, curr_write = write_;
        return curr_write > curr_read ? curr_write - curr_read : curr_write < curr_read ? mod_ - curr_read + curr_write : 0;
    }
    idx_type capacity() const { return mod_ - size() - 1; }

    bool read(T& val) {
        idx_type curr_read = read_;
        if(curr_read == write_) return false;
        val = buffer_[curr_read];
        read_ = (curr_read + 1) % mod_;
        return true;
    }

    size_t read(T* ptr, size_t len) {
        idx_type curr_read = read_, curr_write = write_;
        idx_type new_read = curr_read + len;
        idx_type mod_read = new_read % mod_;
        if(curr_read == curr_write) {
            return 0;
        } else if(curr_read < curr_write) {
            int l = std::min<int>(int(curr_read) + int(len), curr_write);
            std::copy(buffer_.begin()+curr_read, buffer_.begin()+l, ptr);
            read_ = l;
            return len;
        } else {
            if(mod_read < curr_write) {
                int d = mod_ - curr_read;
                std::copy(buffer_.begin()+curr_read, buffer_.begin()+mod_, ptr);
                std::copy(buffer_.begin(), buffer_.begin()+(len - d), ptr+d);
                read_ = mod_read;
                return len;
            } else if(mod_read == new_read) {
                std::copy(buffer_.begin()+curr_read, buffer_.begin()+curr_read+len, ptr);
                read_ += len;
                return len;
            }
            return 0;
        }
    }

    // This is a non-overwriting ring_buffer
    bool write(const T& val) {
        idx_type curr_write = write_;
        if(read_ == (curr_write + 1) % mod_) return false;
        buffer_[curr_write] = val;
        write_ = (curr_write + 1) % mod_;
        return true;
    }

    // Non-overwriting block copy (no partial writes)
    bool write(T* ptr, size_t len) {
        idx_type curr_read = read_, curr_write = write_;
        idx_type new_write = curr_write + len;
        idx_type mod_write = new_write % mod_;
        if(curr_write >= curr_read) { // Empty or write not yet wrapped
            if(new_write < mod_) {
                std::copy(ptr, ptr+len, buffer_.begin()+curr_write);
                write_ = new_write;
            } else if(mod_write >= curr_read) {
                return false;
            } else {
                int d = mod_ - curr_write;
                std::copy(ptr, ptr+d, buffer_.begin()+curr_write);
                int ww = int(len) - d;
                if(ww > 0) std::copy(ptr+d, ptr+d+ww, buffer_.begin());
                write_ = ww;
            }
        } else if(curr_write < curr_read) { // Write has wrapped and we're nearly full
            if(new_write < curr_read) {
                std::copy(ptr, ptr+len, buffer_.begin()+curr_write);
                write_ = new_write;
            } else return false;    // Cannot do a partial write
        } else {    // We have to handle a wrap
            if(mod_write < curr_read) { // There is sufficient write space
                int d = mod_ - curr_write;
                std::copy(ptr, ptr+d, buffer_.begin()+curr_write);
                int ww = int(len) - d;
                if(ww > 0) std::copy(ptr+d, ptr+d+ww, buffer_.begin());
                write_ = ww;
            } else return false;    // Cannot do a partial write
        }
        return true;
    }

    bool peek(T& val) const {
        idx_type curr_read = read_;
        if(curr_read == write_) return false;
        val = buffer_[curr_read];
        return true;
    }

    bool peek(idx_type idx, T& val) const {
        auto temp_read = (read_ + idx) % mod_;
        if(temp_read == write_) return false;
        val = buffer_[temp_read];
        return true;
    }

    bool skip() {
        idx_type curr_read = read_;
        if(curr_read == write_) return false;
        read_ = (curr_read + 1) % mod_;
        return true;
    }

    idx_type skip(idx_type len) {
        idx_type new_read = read_ + len;
        idx_type mod_read = new_read % mod_;
        idx_type curr_write = write_;
        if(read_ == curr_write) {
            return 0;
        } else if(read_ < curr_write) {
            int l = std::min<int>(int(read_) + int(len), curr_write);
            read_ = l;
            return len;
        } else {
            if(mod_read < curr_write) {
                int d = mod_ - read_;
                read_ = mod_read;
                return len;
            } else if(mod_read == new_read) {
                read_ += len;
                return len;
            }
            return 0;
        }
    }

protected:
    std::vector<T> buffer_;
    cursor_type read_;
    cursor_type write_;
    idx_type mod_;
};






















/* OLD RING_BUFFER

// Note: This is an overwriting ring buffer.  If the buffer is full, it will start overwriting the oldest data.
// The implementation may optionally check the size of the buffer prior to writing to implement queueing; however, for
// audio, it usually makes sense to just overwrite.

template <typename T>
class ring_buffer {
public:
    static_assert(std::is_trivially_constructible<T>::value && std::is_copy_constructible<T>::value,
                  "ring_buffer<> type requires trivial construction and copy construction");
    ring_buffer() : mod_(0), read_(0), write_(0) { }
    ring_buffer(size_t mod) : mod_(mod), read_(0), write_(0), buffer_(mod) { }
    ~ring_buffer() = default;

    void resize(size_t mod) { mod_ = mod; buffer_.resize(mod); clear(); }

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
            auto old_read = read_;
            std::copy(buffer_.begin() + read_, buffer_.begin() + write_, ptr);
            read_ = write_;
            return write_ - old_read;
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
            auto old_read = read_;
            std::copy(buffer_.begin() + read_, buffer_.begin() + write_, ptr);
            read_ = write_;
            return write_ - old_read;
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
            auto old_read = read_;
            read_ = write_;
            return write_ - old_read;
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
    size_t mod_;
    size_t read_;
    size_t write_;
    std::vector<T> buffer_;
};
*/

#endif //ZAPAUDIO_RING_BUFFER_HPP
