#include "audio_output.hpp"
#include <thread>
#include <mutex>
#include <portaudio.h>
#include "../log.hpp"
#include "ring_buffer.hpp"
#include "sine_wave.hpp"
#include "mp3_stream.hpp"

template <typename SampleT>
struct audio_output<SampleT>::state_t {
    std::thread audio_thread;
    ring_buffer<SampleT> sample_buffer;
    stream_t* stream_ptr;
};

typedef int callback(const void* input, void* output, u_long frame_count,
    const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
    void* userdata);

int audio_output_callback_s16(const void* input, void* output, u_long frame_count,
        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
        void* userdata) {
    using stream_t = audio_output_s16::stream_t;

    short* out = static_cast<short*>(output);
    stream_t* stream_ptr = static_cast<stream_t*>(userdata);

    if(!stream_ptr) {
        SM_LOG("Null stream passed. Aborting");
        return paAbort;
    }

    typename stream_t::buffer_t buffer(1024);
    auto len = stream_ptr->read(buffer, 1024);
    if(len == 0) {
        SM_LOG("Complete");
        return paComplete;
    }

    for(size_t i = 0; i != 1024; ++i) *out++ = buffer[i];
    return paContinue;
}

template <typename SampleT>
void audio_output<SampleT>::audio_thread_fnc(audio_output* parent_ptr) {
    SM_LOG("Starting");
    if(Pa_Initialize() != paNoError) {
        SM_LOG("Error initialising portaudio");
        return;
    }

    PaStreamParameters output_parms;
    output_parms.device = Pa_GetDefaultOutputDevice();
    if(output_parms.device == paNoDevice) {
        SM_LOG("Portaudio could not find a default playback device");
        return;
    }

    PaStream* pa_stream;
    PaError err = Pa_OpenDefaultStream(
            &pa_stream,
            0,
            2,
            paInt16,
            44100,
            512,
            &audio_output_callback_s16,
            parent_ptr->s.stream_ptr
    );

    if(err != paNoError) {
        SM_LOG("Pa_OpenStream failed:", err, Pa_GetErrorText(err));
        if(pa_stream) Pa_CloseStream(pa_stream);
        return;
    }

    if(Pa_StartStream(pa_stream) != paNoError) {
        SM_LOG("Pa_StartStream failed:", err, Pa_GetErrorText(err));
        if(pa_stream) Pa_CloseStream(pa_stream);
        return;
    }

    while(Pa_IsStreamActive(pa_stream) > 0) Pa_Sleep(1000);

    SM_LOG("Finished");

    parent_ptr->output_state_ = audio_state::AS_STOPPED;

    if(Pa_Terminate() != paNoError) {
        SM_LOG("Pa_Terminate error:", err, Pa_GetErrorText(err));
        return;
    }
}

template <typename SampleT>
audio_output<SampleT>::audio_output() : output_state_(audio_state::AS_STOPPED), state_(new state_t()), s(*state_) {
    //s.stream_ptr = new sine_wave<SampleT>(440);
    auto mp3 = new mp3_stream("/Users/otgaard/Development/prototypes/simple_mp3/output/assets/aphextwins.mp3", 1024, nullptr);
    mp3->start();
    s.stream_ptr = mp3;
}

template <typename SampleT>
audio_output<SampleT>::~audio_output() {
    if(is_playing() || is_paused()) {
        SM_LOG("Warning: audio_output was not stopped");
        stop();
    }

    s.audio_thread.join();
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

    s.audio_thread = std::move(std::thread(audio_output<SampleT>::audio_thread_fnc, this));

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
//template class audio_output<float>;
