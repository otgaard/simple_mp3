/* Created by Darren Otgaar on 2016/11/19. http://www.github.com/otgaard/zap */
#include "buffered_stream.hpp"
#include "../log.hpp"

template <typename SampleT>
buffered_stream<SampleT>::buffered_stream(size_t buffer_size, size_t refill, size_t scan_freq,
    audio_stream<SampleT>* parent) : audio_stream<SampleT>(parent), buffer_(buffer_size), refill_(refill),
                                     scan_freq_(scan_freq), shutdown_(true) {
}

template <typename SampleT>
buffered_stream<SampleT>::~buffered_stream() {
    SM_LOG("Shutting down buffered stream");
    if(shutdown_ == true) return;   // The stream is already closed

    shutdown_ = true;
    scan_thread_.join();
}

template <typename SampleT>
bool buffered_stream<SampleT>::start() {
    shutdown_ = false;
    scan_thread_ = std::thread(buffered_stream<SampleT>::scan_thread, this);
    return true;
}

template <typename SampleT>
size_t buffered_stream<SampleT>::read(buffer_t& buffer, size_t len) {
    return buffer_.read(buffer.data(), len);
}

template <typename SampleT>
size_t buffered_stream<SampleT>::write(const buffer_t& buffer, size_t len) {
    return 0;
}

template <typename SampleT>
void buffered_stream<SampleT>::scan_thread(buffered_stream* ptr) {
    if(!ptr) return;

    // Setup the state
    audio_stream<SampleT>* parent_ptr = ptr->parent();
    if(!parent_ptr) {
        SM_LOG("Error buffering null stream");
        return;
    }

    size_t cache_cur = 0;
    typename audio_stream<SampleT>::buffer_t cache(ptr->refill_);
    const auto scan_ms = std::chrono::milliseconds(ptr->scan_freq_);

    while(!ptr->shutdown_) {
        if(cache_cur == 0) {
            // Refill the cache
            cache_cur = parent_ptr->read(cache, ptr->refill_);
        }

        if(ptr->buffer_.size() < ptr->refill_) {
            ptr->buffer_.write(cache.data(), cache_cur);
            cache_cur = 0;
        }

        std::this_thread::sleep_for(scan_ms);
    }
}

template class ZAPAUDIO_EXPORT buffered_stream<short>;
template class ZAPAUDIO_EXPORT buffered_stream<float>;