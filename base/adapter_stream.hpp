//
// Created by Darren Otgaar on 2016/11/26.
//

#ifndef ZAPAUDIO_ADAPTER_STREAM_HPP
#define ZAPAUDIO_ADAPTER_STREAM_HPP

#include <limits>
#include "audio_stream.hpp"

template <typename OutSampleT, typename InSampleT> OutSampleT sample_convert(InSampleT sample);

template <>
float sample_convert<float, short>(short sample) {
    static const float inv = 1.f/std::numeric_limits<short>::max();
    return sample * inv;
}

template <typename OutSampleT, typename InSampleT>
class adapter_stream : public audio_stream<OutSampleT> {
public:
    using in_stream_t = audio_stream<InSampleT>;
    using out_stream_t = audio_stream<OutSampleT>;
    using buffer_t = typename audio_stream<OutSampleT>::buffer_t;

    adapter_stream(audio_stream<InSampleT>* in_stream) : in_stream_(in_stream) { }

    virtual size_t read(buffer_t& buffer, size_t len) override {
        if(in_buffer_.size() != len) in_buffer_.resize(len);
        auto ret = in_stream_->read(in_buffer_, len);
        for(int i = 0; i != ret; ++i) buffer[i] = sample_convert<float>(in_buffer_[i]);
        return ret;
    }

    virtual size_t write(const buffer_t& buffer, size_t len) override {
        return 0;
    }

private:
    typename in_stream_t::buffer_t in_buffer_;
    audio_stream<InSampleT>* in_stream_;
};

#endif //ZAPAUDIO_ADAPTER_STREAM_HPP
