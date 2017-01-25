/* Created by Darren Otgaar on 2016/11/19. http://www.github.com/otgaard/zap */
#ifndef ZAPAUDIO_BUFFERED_STREAM_HPP
#define ZAPAUDIO_BUFFERED_STREAM_HPP

/*
 * Creates a buffered stream to avoid blocking on I/O or long-running processes.
 */

#include <thread>
#include <atomic>
#include <mutex>
#include "audio_stream.hpp"
#include "buffers/ring_buffer.hpp"

template <typename SampleT>
class ZAPAUDIO_EXPORT buffered_stream : public audio_stream<SampleT> {
public:
    using stream_t = audio_stream<SampleT>;
    using buffer_t = typename stream_t::buffer_t;

    buffered_stream(size_t buffer_size, size_t refill, size_t scan_freq, audio_stream<SampleT>* parent);
    virtual ~buffered_stream();

    bool start();

    virtual size_t read(buffer_t& buffer, size_t len) override final;
    virtual size_t write(const buffer_t& buffer, size_t len) override final;

protected:
    static void scan_thread(buffered_stream* ptr);

private:
    ring_buffer<SampleT, int> buffer_;
    size_t refill_;
    size_t scan_freq_;
    std::thread scan_thread_;
    std::atomic<bool> shutdown_;
};

#endif //ZAPAUDIO_BUFFERED_STREAM_HPP
