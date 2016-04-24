//
// Created by Darren Otgaar on 2016/01/28.
//

#ifndef SIMPLE_MP3_AUDIO_STREAM_H
#define SIMPLE_MP3_AUDIO_STREAM_H

#include <vector>

template <typename T>
class audio_stream {
public:
    using sample_t = T;
    using buffer_t = std::vector<T>;

    audio_stream(audio_stream<T>* parent=nullptr) : parent_(parent) { }
    virtual ~audio_stream() { }

    virtual size_t read(buffer_t& buffer, size_t len) = 0;
    virtual size_t write(const buffer_t& buffer, size_t len) = 0;

private:
    audio_stream<T>* parent_;
};


#endif //SIMPLE_MP3_AUDIO_STREAM_H
