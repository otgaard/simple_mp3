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
    size_t channel_frame_size;
    size_t buffer_size;

    ring_buffer<SampleT> sample_buffer;
    stream_t* stream_ptr;
    std::mutex context_mtx;
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
    size_t len = 0;

    {
        std::lock_guard<std::mutex> lock(context_ptr->context_mtx);
        len = context_ptr->sample_buffer.read(buffer.data(), context_ptr->buffer_size);
    }

    if(len == 0 || context_ptr->shutdown) {
        SM_LOG("Complete");
        return paComplete;
    }

    for(size_t i = 0; i != len; ++i) *out++ = buffer[i];
    return paContinue;
}

template <typename SampleT>
void audio_output<SampleT>::audio_thread_fnc(audio_output* parent_ptr) {
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
            data_type_table<SampleT>::value,
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

    // Initialise the ring_buffer

    // 44100 sample rate x 2 channels x 2 bytes per sample = 176400 bytes per second = 172 kB per second

    const size_t buf_size = 64*parent_ptr->frame_size();
    const size_t hbuf_size = buf_size / 2;
    const auto scan_ms = std::chrono::milliseconds(60);

    auto& context = parent_ptr->s.context;
    size_t cache_cur = 0;                                   // The cache cursor
    context.sample_buffer.resize(buf_size);                 // 64 kB
    typename stream_t::buffer_t sample_cache(hbuf_size);    // 32 kB

    while(Pa_IsStreamActive(pa_stream) > 0) {
        if(cache_cur == 0) {
            // Refill the cache with samples from the stream, the stream may possibly block on I/O
            cache_cur = context.stream_ptr->read(sample_cache, hbuf_size);
        }

        // Check, every 60 milliseconds, if the sample_buffer needs refilling
        {
            std::lock_guard<std::mutex> lock(context.context_mtx);
            if(context.sample_buffer.size() < context.sample_buffer.capacity()/2) { // If sample_buffer < 1/2
                context.sample_buffer.write(sample_cache.data(), cache_cur);
                cache_cur = 0;
            }
        }

        std::this_thread::sleep_for(scan_ms);
    }

    parent_ptr->audio_state_ = audio_state::AS_STOPPED;

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
    if(is_playing() || is_paused()) {
        SM_LOG("Warning: audio_output was not stopped");
        stop();
    }
}

template <typename SampleT>
void audio_output<SampleT>::set_stream(stream_t* stream_ptr) {
    if(is_stopped()) {
        s.context.stream_ptr = stream_ptr;
        s.context.sample_buffer.clear();
        s.context.shutdown = false;
    }
}

template <typename SampleT>
typename audio_output<SampleT>::stream_t* audio_output<SampleT>::get_stream() const {
    return s.context.stream_ptr;
}

template <typename SampleT>
void audio_output<SampleT>::play() {
    if(is_playing() || is_paused()) return;
    SM_LOG("Starting audio_output");

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

template class audio_output<short>;
//template class audio_output<float>;
