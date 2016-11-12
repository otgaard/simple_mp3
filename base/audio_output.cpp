#include "audio_output.hpp"
#include <thread>
#include <mutex>
#include <portaudio.h>
#include "../log.hpp"
#include "ring_buffer.hpp"
#include "sine_wave.hpp"

template <typename SampleT>
struct audio_output<SampleT>::state_t {
    std::thread audio_thread;
    ring_buffer<SampleT> sample_buffer;
    stream_t* stream_ptr;
};

template <typename SampleT>
void audio_output<SampleT>::audio_thread_fnc(audio_output* parent_ptr) {

}

template <typename SampleT>
audio_output<SampleT>::audio_output() : output_state_(audio_state::AS_STOPPED), state_(new state_t()), s(*state_) {
    s.stream_ptr = new sine_wave<SampleT>(440);
}

template <typename SampleT>
audio_output<SampleT>::~audio_output() {
    if(is_playing() || is_paused()) {
        SM_LOG("Warning: audio_output was not stopped");
        stop();
    }
}

template <typename SampleT>
void audio_output<SampleT>::set_stream(stream_t* stream_ptr) {
    if(is_stopped()) s.stream_ptr = stream_ptr;
}

template <typename SampleT>
typename audio_output<SampleT>::stream_t* audio_output<SampleT>::get_stream() const {
    return s.stream_ptr;
}

template <typename SampleT>
bool audio_output<SampleT>::play() {
    SM_LOG("Starting audio_output");



    output_state_ = audio_state::AS_PLAYING;
}

template <typename SampleT>
void audio_output<SampleT>::pause() {
    SM_LOG("Pausing audio_output");

    output_state_ = audio_state::AS_PAUSED;
}

template <typename SampleT>
void audio_output<SampleT>::stop() {
    SM_LOG("Stopping audio_output");

    output_state_ = audio_state::AS_STOPPED;
}

template class audio_output<short>;
template class audio_output<float>;
