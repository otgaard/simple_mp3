#include "audio_output.hpp"
#include <thread>
#include <mutex>
#include <portaudio.h>
#include "../log.hpp"
#include "ring_buffer.hpp"
#include "sine_wave.hpp"
#include "mp3_stream.hpp"

template <typename SampleT>
struct data_type_table;

template <> struct data_type_table<short> { enum { value = paInt16 }; };
template <> struct data_type_table<float> { enum { value = paFloat32 }; };

template <typename SampleT>
struct audio_context {
    using stream_t = typename audio_output<SampleT>::stream_t;
    using buffer_t = typename stream_t::buffer_t;

    size_t channels;
    size_t sample_rate;
    size_t frame_size;
    size_t buffer_size;

    ring_buffer<SampleT> sample_buffer;
    stream_t* stream_ptr;
    std::mutex context_mtx;
    std::atomic<bool> shutdown;

    audio_context() : stream_ptr(nullptr) { }
};


template <typename SampleT>
struct audio_output<SampleT>::state_t {
    std::thread audio_thread;
    audio_context<SampleT> context;
};

typedef int callback(const void* input, void* output, u_long frame_count,
    const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
    void* userdata);

int audio_output_callback_s16(const void* input, void* output, u_long frame_count,
        const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
        void* userdata) {
    short* out = static_cast<short*>(output);
    using context = audio_context<short>;
    context* context_ptr = static_cast<context*>(userdata);

    if(!context_ptr || !context_ptr->stream_ptr) {
        SM_LOG("Null context or stream passed. Aborting");
        return paAbort;
    }

    static typename context::buffer_t buffer(context_ptr->buffer_size);
    auto len = context_ptr->stream_ptr->read(buffer, context_ptr->buffer_size);
    if(len == 0 || context_ptr->shutdown) {
        SM_LOG("Complete");
        return paComplete;
    }

    for(size_t i = 0; i != context_ptr->buffer_size; ++i) *out++ = buffer[i];
    return paContinue;
}

template <typename SampleT>
void audio_output<SampleT>::audio_thread_fnc(audio_output* parent_ptr, size_t channels, size_t sample_rate, size_t frame_size) {
    SM_LOG("Starting", channels, sample_rate, frame_size);

    auto context_ptr = &parent_ptr->s.context;

    context_ptr->channels = channels;
    context_ptr->sample_rate = sample_rate;
    context_ptr->frame_size = frame_size/channels;
    context_ptr->buffer_size = frame_size;

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
            channels,
            data_type_table<SampleT>::value,
            sample_rate,
            frame_size/channels,
            &audio_output_callback_s16,
            context_ptr
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
    s.context.stream_ptr = mp3;
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
    if(is_stopped()) s.context.stream_ptr = stream_ptr;
}

template <typename SampleT>
typename audio_output<SampleT>::stream_t* audio_output<SampleT>::get_stream() const {
    return s.context.stream_ptr;
}

template <typename SampleT>
bool audio_output<SampleT>::play() {
    SM_LOG("Starting audio_output");

    s.audio_thread = std::move(std::thread(audio_output<SampleT>::audio_thread_fnc, this, 2, 44100, 1024));

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

    s.context.shutdown = true;
    s.audio_thread.join();

    output_state_ = audio_state::AS_STOPPED;
}

template class audio_output<short>;
//template class audio_output<float>;
