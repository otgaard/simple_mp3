#ifndef ZAPAUDIO_AUDIO_OUTPUT_HPP
#define ZAPAUDIO_AUDIO_OUTPUT_HPP

#include <memory>
#include "streams/audio_stream.hpp"

/*
 * audio_output is the output device and interface to portaudio.
 */

template <typename SampleT>
class ZAPAUDIO_EXPORT audio_output {
public:
    enum class audio_state {
        AS_STOPPED,
        AS_PLAYING,
        AS_PAUSED,
        AS_COMPLETED
    };

    using sample_t = SampleT;
    using buffer_t = std::vector<SampleT>;
    using stream_t = audio_stream<SampleT>;

    audio_output(stream_t* stream_ptr, size_t channels=2, size_t sample_rate=44100, size_t frame_size=1024);
    ~audio_output();

    void set_stream(stream_t* stream_ptr);
    stream_t* get_stream() const;

    void play();
    void pause();
    void stop();

    bool is_playing() { return audio_state_ == audio_state::AS_PLAYING; }
    bool is_paused() { return audio_state_ == audio_state::AS_PAUSED; }
    bool is_stopped() { return audio_state_ == audio_state::AS_STOPPED; }
    bool is_completed() { return audio_state_ == audio_state::AS_COMPLETED; }

    size_t channels() const { return channels_; }
    size_t sample_rate() const { return sample_rate_; }
    size_t frame_size() const { return frame_size_; }

    audio_state get_state() const { return audio_state_; }

protected:
    audio_state audio_state_;
    size_t channels_;
    size_t sample_rate_;
    size_t frame_size_;

    static void audio_thread_fnc(audio_output* parent_ptr);

private:
    struct state_t;
    std::unique_ptr<state_t> state_;
    state_t& s;
};

using audio_output_s16 = audio_output<short>;
using audio_output_f32 = audio_output<float>;

#endif //ZAPAUDIO_AUDIO_OUTPUT_HPP
