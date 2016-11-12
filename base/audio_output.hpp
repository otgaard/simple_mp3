#ifndef ZAPAUDIO_AUDIO_OUTPUT_HPP
#define ZAPAUDIO_AUDIO_OUTPUT_HPP

#include <memory>
#include "audio_stream.hpp"

/*
 * audio_output is the "device" that makes streams audible.
 */

template <typename SampleT>
class audio_output {
public:
    enum class audio_state {
        AS_STOPPED,
        AS_PLAYING,
        AS_PAUSED
    };

    using sample_t = SampleT;
    using buffer_t = std::vector<SampleT>;
    using stream_t = audio_stream<SampleT>;

    audio_output();
    ~audio_output();

    void set_stream(stream_t* stream_ptr);
    stream_t* get_stream() const;

    bool play();
    void pause();
    void stop();

    bool is_playing() { return output_state_ == audio_state::AS_PLAYING; }
    bool is_paused() { return output_state_ == audio_state::AS_PAUSED; }
    bool is_stopped() { return output_state_ == audio_state::AS_STOPPED; }

    audio_state get_state() const { return output_state_; }

protected:
    audio_state output_state_;

    static void audio_thread_fnc(audio_output* parent_ptr);

private:
    struct state_t;
    std::unique_ptr<state_t> state_;
    state_t& s;
};

using audio_output_s16 = audio_output<short>;
//using audio_output_f32 = audio_output<float>;

#endif //ZAPAUDIO_AUDIO_OUTPUT_HPP
