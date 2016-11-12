//
// Created by Darren Otgaar on 2016/11/12.
//

#ifndef ZAPAUDIO_SINE_WAVE_HPP
#define ZAPAUDIO_SINE_WAVE_HPP

#include <cassert>
#include "audio_stream.hpp"

/*
 * An audio_stream implementation of a 440 Hz sine wave.
 */

template <typename SampleT>
class sine_wave : public audio_stream<SampleT> {
public:
    sine_wave(size_t hertz, size_t sample_rate=44100, size_t channels=2, audio_stream<SampleT>* parent=nullptr)
            : audio_stream<SampleT>(parent), hertz_(hertz), sample_rate_(44100), channels_(channels) {

        step_ = sample_rate / hertz;
    }
    virtual ~sine_wave() = default;

    virtual size_t read(typename audio_stream<SampleT>::buffer_t& buffer, size_t len) {
        for(size_t i = 0, end = len/channels_; i != end; ++i) {

            // Sawtooth wave for now
            curr_sample_ += step_;

            for(int ch = 0; ch != channels_; ++ch) {
                buffer[channels_*i + ch] = curr_sample_;
            }
        }
        return len;
    }

    virtual size_t write(const typename audio_stream<SampleT>::buffer_t& buffer, size_t len) {
        return 0;
    }

private:
    size_t hertz_;
    size_t sample_rate_;
    size_t channels_;
    SampleT curr_sample_;
    SampleT step_;
};

#endif //ZAPAUDIO_SINE_WAVE_HPP
