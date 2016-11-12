//
// Created by Darren Otgaar on 2016/01/28.
//

#ifndef ZAPAUDIO_AUDIO_STREAM_H
#define ZAPAUDIO_AUDIO_STREAM_H

#include <vector>

template <typename SampleT>
class audio_stream {
public:
    using sample_t = SampleT;
    using buffer_t = std::vector<SampleT>;

    audio_stream(audio_stream<SampleT>* parent=nullptr) : parent_(parent) { }
    virtual ~audio_stream() { }

    virtual size_t read(buffer_t& buffer, size_t len) = 0;
    virtual size_t write(const buffer_t& buffer, size_t len) = 0;

private:
    audio_stream<SampleT>* parent_;
};

#endif //ZAPAUDIO_AUDIO_STREAM_H
