//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef ZAPAUDIO_RING_BUFFER_HPP
#define ZAPAUDIO_RING_BUFFER_HPP

#include <atomic>
#include <vector>
#include <limits>
#include <cassert>

template <typename IndexT, bool IsAtomic>
struct index_traits;

template <typename IndexT>
struct index_traits<IndexT, true> {
    using value_t = IndexT;
    using index_t = std::atomic<IndexT>;
    static inline value_t load(const index_t& idx) { return idx.load(std::memory_order_relaxed); }
    static inline void store(index_t& idx, value_t value) { idx.store(value, std::memory_order_relaxed); }
};

template <typename IndexT>
struct index_traits<IndexT, false> {
    using value_t = IndexT;
    using index_t = IndexT;
    static inline value_t load(const index_t& idx) { return idx; }
    static inline void store(index_t& idx, value_t value) { idx = value; }
};


template <typename T, typename IndexT, bool IsAtomic=true, typename IndexTraits=index_traits<IndexT, IsAtomic>>
class ring_buffer {
public:
    static_assert(std::is_integral<IndexT>::value, "IndexT must be integral type");

    using type = T;
    using ptr_t = T*;
    using idx_type = IndexT;
    using cursor_type = typename IndexTraits::index_t;

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
        idx_type curr_read = IndexTraits::load(read_);
        if(curr_read == IndexTraits::load(write_)) return false;
        val = buffer_[curr_read];
        IndexTraits::store(read_, (curr_read + 1) % mod_);
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
        idx_type curr_write = IndexTraits::load(write_);
        idx_type new_write = (curr_write + 1) % mod_;
        if(IndexTraits::load(read_) == new_write) return false;
        buffer_[curr_write] = val;
        IndexTraits::store(write_, new_write);
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

#endif //ZAPAUDIO_RING_BUFFER_HPP
