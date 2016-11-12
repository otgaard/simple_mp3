#include "audio_output.hpp"
#include <thread>
#include <mutex>
#include <portaudio.h>

struct audio_output::state_t {
    std::thread audio_thread;

};

template <typename SampleT>
audio_output::audio_output() {

}

template <typename SampleT>
audio_output::~audio_output() {

}

template <typename SampleT>
void audio_output::set_stream(stream_t* stream_ptr) {

}

template <typename SampleT>
stream_t* audio_output::get_stream() const {

}

template <typename SampleT>
void audio_output::play() {



}

template <typename SampleT>
void audio_output::pause() {

}

template <typename SampleT>
void audio_output::stop() {

}
