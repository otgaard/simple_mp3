//
// Created by Darren Otgaar on 2016/11/12.
//

#ifndef ZAPAUDIO_SINE_WAVE_HPP
#define ZAPAUDIO_SINE_WAVE_HPP

#include <cmath>
#include <cassert>
#include "audio_stream.hpp"

/*
 * An audio_stream implementation of a sine wave generator.
 */

template <typename SampleT>
class sine_wave : public audio_stream<SampleT> {
public:
    sine_wave(size_t hertz, size_t sample_rate=44100, size_t channels=2, audio_stream<SampleT>* parent=nullptr)
            : audio_stream<SampleT>(parent), hertz_(hertz), sample_rate_(44100), channels_(channels), curr_sample_(0) {
        period_ = std::max(sample_rate / hertz, size_t(1));
    }
    virtual ~sine_wave() = default;

    void set_hertz(size_t hertz) {
        hertz_ = hertz;
        period_ = std::max(sample_rate_ / hertz, size_t(1));
    }

    size_t get_hertz() const { return hertz_; }

    virtual size_t read(typename audio_stream<SampleT>::buffer_t& buffer, size_t len) {
        constexpr double PI = 3.14159265358979323846;
        constexpr double TWO_PI = 2.0 * PI;

        for(size_t i = 0, end = len/channels_; i != end; ++i) {

            float x = curr_sample_ / float(period_);
            short value = short(std::round(std::numeric_limits<short>::max()*std::sin(TWO_PI * x)));

            for(int ch = 0; ch != channels_; ++ch) buffer[channels_*i + ch] = value;

            curr_sample_ = (curr_sample_ + 1) % period_;
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
    size_t period_;
    size_t curr_sample_;
};

#endif //ZAPAUDIO_SINE_WAVE_HPP
