//
// Created by Darren Otgaar on 2016/11/12.
//

#ifndef ZAPAUDIO_SINE_WAVE_HPP
#define ZAPAUDIO_SINE_WAVE_HPP

#include <cmath>
#include <cassert>
#include "audio_stream.hpp"

constexpr double PI = 3.14159265358979323846;
constexpr double TWO_PI = 2.0 * PI;

/*
 * An audio_stream implementation of a 440 Hz sine wave.
 */

// A saturated add

short sadd(const short& lhs, const short& rhs) {
    if(lhs >= 0) {
        if(std::numeric_limits<short>::max() - lhs < rhs) {
            return std::numeric_limits<short>::max();
        }
    } else {
        if(rhs < std::numeric_limits<short>::lowest() - lhs) {
            return std::numeric_limits<short>::lowest();
        }
    }

    return lhs + rhs;
}

template <typename SampleT>
class sine_wave : public audio_stream<SampleT> {
public:
    sine_wave(size_t hertz, size_t sample_rate=44100, size_t channels=2, audio_stream<SampleT>* parent=nullptr)
            : audio_stream<SampleT>(parent), hertz_(hertz), sample_rate_(44100), channels_(channels), curr_sample_(0) {

        period_ = std::max(sample_rate / hertz, size_t(1));
        SM_LOG("Period", period_);
    }
    virtual ~sine_wave() = default;

    virtual size_t read(typename audio_stream<SampleT>::buffer_t& buffer, size_t len) {
        for(size_t i = 0, end = len/channels_; i != end; ++i) {

            // Square wave for now
            short value1 = curr_sample_ < (period_ / 2) ? std::numeric_limits<short>::max() : std::numeric_limits<short>::lowest();

            // Plus a sine wave
            float x = curr_sample_ / float(period_);
            short value2 = short(std::numeric_limits<short>::max()*std::sin(TWO_PI * x));

            for(int ch = 0; ch != channels_; ++ch) {
                buffer[channels_*i + ch] = sadd(value1/60, value2);
            }

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
