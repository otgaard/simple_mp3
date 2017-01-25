#include "audio_output.hpp"
#include <atomic>
#include <thread>
#include <mutex>
#include <portaudio.h>
#include "log.hpp"
#include "buffers/ring_buffer.hpp"

#ifdef _WIN32
typedef unsigned long u_long;
#endif

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
    size_t channel_frame_size;
    size_t buffer_size;

    stream_t* stream_ptr;
    std::atomic<bool> shutdown;

    audio_context() : stream_ptr(nullptr) { }
};


template <typename SampleT>
struct audio_output<SampleT>::state_t {
    std::unique_ptr<std::thread> audio_thread_ptr;
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

    if(!context_ptr) {
        SM_LOG("Null context passed. Aborting");
        return paAbort;
    }

    static typename context::buffer_t buffer(context_ptr->buffer_size);
    size_t len = context_ptr->stream_ptr->read(buffer, context_ptr->buffer_size);

    if(len == 0 || context_ptr->shutdown) {
        SM_LOG("Complete");
        return paComplete;
    }

    for(size_t i = 0; i != len; ++i) *out++ = buffer[i];
    return paContinue;
}

int audio_output_callback_f32(const void* input, void* output, u_long frame_count,
                              const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                              void* userdata) {
    float* out = static_cast<float*>(output);
    using context = audio_context<float>;
    context* context_ptr = static_cast<context*>(userdata);

    if(!context_ptr) {
        SM_LOG("Null context passed. Aborting");
        return paAbort;
    }

    static typename context::buffer_t buffer(context_ptr->buffer_size);
    size_t len = context_ptr->stream_ptr->read(buffer, context_ptr->buffer_size);

    if(len == 0 || context_ptr->shutdown) {
        SM_LOG("Complete");
        return paComplete;
    }

    for(size_t i = 0; i != len; ++i) *out++ = buffer[i];
    return paContinue;
}

template <>
void audio_output<short>::audio_thread_fnc(audio_output* parent_ptr) {
    SM_LOG("Starting", parent_ptr->channels(), parent_ptr->sample_rate(), parent_ptr->frame_size());

    auto context_ptr = &parent_ptr->s.context;

    context_ptr->channels = parent_ptr->channels();
    context_ptr->sample_rate = parent_ptr->sample_rate();
    context_ptr->channel_frame_size = parent_ptr->frame_size()/parent_ptr->channels();
    context_ptr->buffer_size = parent_ptr->frame_size();

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
            int(parent_ptr->channels()),
            data_type_table<short>::value,
            parent_ptr->sample_rate(),
            context_ptr->channel_frame_size,
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

    while(Pa_IsStreamActive(pa_stream) > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Should lock this (atomic type?)
    parent_ptr->audio_state_ = audio_state::AS_COMPLETED;

    if(Pa_Terminate() != paNoError) {
        SM_LOG("Pa_Terminate error:", err, Pa_GetErrorText(err));
        return;
    }
}

template <>
void audio_output<float>::audio_thread_fnc(audio_output* parent_ptr) {
    SM_LOG("Starting", parent_ptr->channels(), parent_ptr->sample_rate(), parent_ptr->frame_size());

    auto context_ptr = &parent_ptr->s.context;

    context_ptr->channels = parent_ptr->channels();
    context_ptr->sample_rate = parent_ptr->sample_rate();
    context_ptr->channel_frame_size = parent_ptr->frame_size()/parent_ptr->channels();
    context_ptr->buffer_size = parent_ptr->frame_size();

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
            int(parent_ptr->channels()),
            data_type_table<float>::value,
            parent_ptr->sample_rate(),
            context_ptr->channel_frame_size,
            &audio_output_callback_f32,
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

    while(Pa_IsStreamActive(pa_stream) > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Should lock this (atomic type?)
    parent_ptr->audio_state_ = audio_state::AS_COMPLETED;

    if(Pa_Terminate() != paNoError) {
        SM_LOG("Pa_Terminate error:", err, Pa_GetErrorText(err));
        return;
    }
}

template <typename SampleT>
audio_output<SampleT>::audio_output(stream_t* stream_ptr, size_t channels, size_t sample_rate, size_t frame_size)
        : audio_state_(audio_state::AS_STOPPED), channels_(channels), sample_rate_(sample_rate),
          frame_size_(frame_size), state_(new state_t()), s(*state_) {
    state_->context.stream_ptr = stream_ptr;
}

template <typename SampleT>
audio_output<SampleT>::~audio_output() {
    if(!is_stopped()) {
        SM_LOG("Warning: audio_output was not stopped");
        stop();
    }
}

template <typename SampleT>
void audio_output<SampleT>::set_stream(stream_t* stream_ptr) {
    if(!is_stopped()) stop();

    s.context.stream_ptr = stream_ptr;
    //s.context.sample_buffer.clear();
    s.context.shutdown = false;
}

template <typename SampleT>
typename audio_output<SampleT>::stream_t* audio_output<SampleT>::get_stream() const {
    return s.context.stream_ptr;
}

template <typename SampleT>
void audio_output<SampleT>::play() {
    if(!is_stopped()) return;

    SM_LOG("Starting audio_output");

    if(s.context.stream_ptr == nullptr) {
        SM_LOG("No stream, cannot start audio_output");
        return;
    }

    s.audio_thread_ptr = std::make_unique<std::thread>(audio_output<SampleT>::audio_thread_fnc, this);
    audio_state_ = audio_state::AS_PLAYING;
}

template <typename SampleT>
void audio_output<SampleT>::pause() {
    if(is_stopped()) return;
    SM_LOG("Pausing audio_output");

    audio_state_ = audio_state_ == audio_state::AS_PAUSED ? audio_state::AS_PLAYING : audio_state::AS_PAUSED;
}

template <typename SampleT>
void audio_output<SampleT>::stop() {
    if(is_stopped()) return;
    SM_LOG("Stopping audio_output");

    s.context.shutdown = true;
    s.audio_thread_ptr->join();
    s.audio_thread_ptr.reset(nullptr);

    audio_state_ = audio_state::AS_STOPPED;
}

template class ZAPAUDIO_EXPORT audio_output<short>;
template class ZAPAUDIO_EXPORT audio_output<float>;
