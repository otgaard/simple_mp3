#include <vector>
#include <fstream>
#include <iostream>
#include <numeric>
#include <portaudio.h>
#include "base/wave_stream.hpp"
#include "decoder/decoder.hpp"

#include "log.hpp"
#include "base/mp3_stream.hpp"

const char* filename = "/Users/otgaard/Development/prototypes/simple_mp3/output/assets/chembros.wav";
const char* mp3file = "/Users/otgaard/Development/prototypes/simple_mp3/output/assets/aphextwins.mp3";
const char* mp3file2 = "/Users/otgaard/Development/prototypes/simple_mp3/output/assets/aphextwins2.mp3";
const char* mp3file3 = "/Users/otgaard/Development/prototypes/simple_mp3/output/assets/opera.mp3";

std::vector<short> buffer(1024);

typedef int callback(const void* input, void* output, u_long frame_count,
                     const PaStreamCallbackTimeInfo* time_info, PaStreamCallbackFlags status_flags,
                     void* userdata);

int wave_callback(const void* input, void* output, u_long frame_count, const PaStreamCallbackTimeInfo* time_info,
                  PaStreamCallbackFlags status_flags, void* userdata) {
    wave_stream* stream = static_cast<wave_stream*>(userdata);
    short* out = static_cast<short*>(output);

    auto len = stream->read(buffer, buffer.size());
    if(len == 0) { std::cerr << "Complete" << std::endl; return paComplete; }
    for(size_t i = 0; i != len; ++i) *out++ = buffer[i];
    return paContinue;
}

int mp3_callback(const void* input, void* output, u_long frame_count, const PaStreamCallbackTimeInfo* time_info,
                 PaStreamCallbackFlags status_flags, void* userdata) {
    //wave_stream* stream = static_cast<wave_stream*>(userdata);
    short* out = static_cast<short*>(output);
    //block_buffer<short>* buf_ptr = static_cast<block_buffer<short>*>(userdata);
    mp3_stream* stream = static_cast<mp3_stream*>(userdata);
    auto len = stream->read(buffer, buffer.size());
    if(len == 0) { std::cerr << "Complete" << std::endl; return paComplete; }
    for(size_t i = 0; i != len; ++i) *out++ = buffer[i];
    return paContinue;
}

block_buffer<short> sample_buffer;

int main(int argc, char*argv[]) {
    /*
    decoder dec;
    dec.initialise();
    sample_buffer = dec.decode_file(mp3file2);
    dec.shutdown();
    LOG("returned from decode_file", sample_buffer.size());
    */

    /*
    wave_stream wstream(filename, 1024, nullptr);
    wstream.start();
    if(!wstream.is_open()) {
        std::cerr << "Error opening WAVE file." << std::endl;
        return -1;
    }
    */

    mp3_stream mstream(mp3file3, 1024, nullptr);
    mstream.start();
    if(!mstream.is_open()) {
        LOG("Error opening mp3");
        return -1;
    }

    if(Pa_Initialize() != paNoError) {
        std::cerr << "Error initialising portaudio" << std::endl;
        return -1;
    }

    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if(outputParameters.device == paNoDevice) {
        std::cerr << "No suitable output device found by portaudio" << std::endl;
        return -1;
    }

    PaStream* stream;

    PaError err = Pa_OpenDefaultStream(
            &stream,
            0,
            2, //wstream.get_header().num_channels,
            paInt16,
            44100, //wstream.get_header().sample_rate,
            512, //512,
            &mp3_callback,
            &mstream //&sample_buffer //&wstream
    );

    if(err != paNoError) {
        std::cerr << "Pa_OpenStream failed:" << err << Pa_GetErrorText(err) << std::endl;
        if(stream) Pa_CloseStream(stream);
        return -1;
    }

    if(Pa_StartStream(stream) != paNoError) {
        std::cerr << "Pa_OpenStream failed:" << err << Pa_GetErrorText(err) << std::endl;
        if(stream) Pa_CloseStream(stream);
        return -1;
    }

    while(Pa_IsStreamActive(stream) > 0) Pa_Sleep(1000);

    if(Pa_Terminate() != paNoError) {
        std::cerr << "Pa_Terminate error:" << err << Pa_GetErrorText(err) << std::endl;
        return -1;
    }

    return 0;
}
