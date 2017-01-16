#include "streams/mp3_stream.hpp"
#include "audio_output.hpp"
#include "streams/buffered_stream.hpp"
#include "streams/adapter_stream.hpp"

const char* const def_filname = "/Users/otgaard/test/another.mp3";

int main(int argc, char* argv[]) {
    // Create an MP3 tools stream
    auto file_stream = std::make_unique<mp3_stream>(argc > 1 ? argv[1] : def_filname, 1024, nullptr);
    file_stream->start();

    // We have to hook up an adapter if we want to play a signed short sample producer in a floating point audio_output.
    auto adapter = std::make_unique<adapter_stream<float, short>>(file_stream.get());

    // Connect the MP3 tools to a buffered stream because the MP3 tools must do I/O.
    const size_t kBUFFER_SIZE = 64*1024;            // The size of the whole buffer
    const size_t kREFILL_SIZE = kBUFFER_SIZE/2;     // The size at which we should refill the buffer
    const size_t kSCAN_MS = 60;                     // The milliseconds between refill scans
    auto buf_stream = std::make_unique<buffered_stream<float>>(kBUFFER_SIZE, kREFILL_SIZE, kSCAN_MS, adapter.get());
    buf_stream->start();

    // Use the buffered stream as the source for the audio device and play.
    audio_output<float> audio_dev(buf_stream.get());
    audio_dev.play();

    while(audio_dev.is_playing()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    audio_dev.stop();

    return 0;
}
