#include "mp3_stream.hpp"
#include <algorithm>
#ifdef _WIN32
#include <lame.h>
#else
#include <lame/lame.h>
#endif //_WIN32
#include "log.hpp"

mp3_format lame_2_header(const mp3data_struct& mp3data);

mp3_stream::mp3_stream(const std::string& filename, size_t frame_size, audio_stream<short>* parent) :
        filename_(filename), header_parsed_(false), file_size_(0), frame_size_(frame_size),
        output_buffer_(128*mp3_frame_size), input_buffer_(128*frame_size), lame_(nullptr) {
    read_buf.resize(frame_size);
}

mp3_stream::~mp3_stream() {
    if(file_.is_open()) file_.close();
    if(lame_) shutdown();
}

bool mp3_stream::start() {
    if(!initialise()) return false;
    file_ = std::ifstream(filename_, std::ios_base::binary | std::ios_base::in);

    if(file_.is_open()) {
        file_.seekg(0, std::ios::end);
        file_size_ = (size_t)file_.tellg();
        file_.seekg(0, std::ios::beg);

        fill_input_buffer();

        // Load some data into the input buffer and try to strip the ID3 header
        if(!strip_header()) {
            if(is_open()) file_.close();
            SM_LOG("Could not strip header");
            return false;
        }

        while(!header_parsed_ && is_open()) {
            fill_output_buffer();
        }
    }
    return true;
}

size_t mp3_stream::read(buffer_t& buffer, size_t len) {
    if(output_buffer_.size() < len) fill_output_buffer();
    auto l = output_buffer_.read(buffer.data(), len);
    return l;
}

size_t mp3_stream::write(const buffer_t& buffer, size_t len) {
    return 0;
}

bool mp3_stream::initialise() {
    lame_ = lame_init();
    if(!lame_) {
        SM_LOG("LAME failed to initialise");
        return false;
    }

    lame_set_decode_only(lame_, 1);
    lame_set_num_samples(lame_, std::numeric_limits<unsigned int>::max());

    hip_ = hip_decode_init();
    if(!hip_) {
        lame_close(lame_);
        lame_ = nullptr;
        SM_LOG("LAME HIP failed to initialise");
        return false;
    }
    return true;
}

void mp3_stream::shutdown() {
    hip_decode_exit(hip_);
    hip_ = nullptr;
    lame_close(lame_);
    lame_ = nullptr;
}

void mp3_stream::fill_input_buffer() {
    while(is_open() && (input_buffer_.size() < input_buffer_.capacity()/2)) {
        auto rd = std::min(size_t(file_size_ - file_.tellg()), frame_size_);
        file_.read(reinterpret_cast<char*>(read_buf.data()), frame_size_);
        size_t off = (size_t)file_.tellg();
        if(off == -1) file_.close();
        else          input_buffer_.write(read_buf.data(), rd);
    }
}

int mp3_stream::zip(short* left_pcm, short* right_pcm, int len) {
    for(int i = 0; i != len; ++i) {
        output_buffer_.write(left_pcm+i, 1);
        output_buffer_.write(right_pcm+i, 1);
    }
    return len;
}

void mp3_stream::fill_output_buffer() {
    static mp3data_struct mp3data;

    int ret = 0, len = 0;
    while(output_buffer_.size() < output_buffer_.capacity()/2 && input_buffer_.size() > 0) {
        len = input_buffer_.read(read_buf.data(), frame_size_);
        if(!header_parsed_) {
            ret = hip_decode1_headers(hip_, read_buf.data(), len, left_pcm, right_pcm, &mp3data);
            if(mp3data.header_parsed) {
                header_ = lame_2_header(mp3data);
                header_parsed_ = true;
            }
        } else {
            ret = hip_decode1_headers(hip_, read_buf.data(), len, left_pcm, right_pcm, &mp3data);
            while(ret != 0) {
                zip(left_pcm, right_pcm, ret);
                ret = hip_decode1_headers(hip_, read_buf.data(), 0, left_pcm, right_pcm, &mp3data);
            }
        }
        fill_input_buffer();
    }
}

bool mp3_stream::strip_header() {
    assert(input_buffer_.read_cursor() == 0 && "buffer should be positioned at zero");
    byte header[100];
    if(input_buffer_.read(header, 4) != 4) return false;

    if(memcmp(header, "ID3", 3) == 0) { // Check ID3 Header
        SM_LOG("ID3 found");
        // Read the length of the id3 header
        if(input_buffer_.read(header, 6) != 6) return false;
        header[2] &= 0x7f; header[3] &= 0x7f; header[4] &= 0x7f; header[5] &= 0x7f;
        size_t skip = (((((header[2] << 7) + header[3]) << 7) + header[4] ) << 7) + header[5];
        SM_LOG("skipping =", skip);

        // The skip size sometimes includes the header
        skip -= 4;

        size_t skipped;
        do {
            skipped = input_buffer_.skip(skip);
            if(skipped < skip) {
                fill_input_buffer();
            }
            skip -= skipped;
        } while(skipped != 0 && skip != 0);

        if(skip != 0)                            return false;
        if(input_buffer_.read(header, 4) != 4)   return false;   // Reposition buffer
    }

    if(memcmp(header, "AiD\1", 4) == 0) { // Check for Album ID Header
        SM_LOG("Album ID found");
        if(input_buffer_.read(header, 2) != 2) return false;
        size_t skip = (size_t)header[0] + 256 * (size_t)header[1];
        if(input_buffer_.skip(skip - 6) != (skip - 6)) return false;
        if(input_buffer_.read(header, 4) != 4) return false;   // Reposition buffer
    }

    // Now scan to find the mp3 sync word
    size_t skipped = 0;
    while(!is_syncword_mp123(input_buffer_.read_ptr())) {
        byte dummy;
        if(input_buffer_.read(&dummy, 1) != 1) return false;
        skipped++;
        if(skipped >= 2048) {
            SM_LOG("Corrupted file, more than 2048 bytes skipped after headers and still no sync word");
            return false;
        }
    }
    return true;
}

bool mp3_stream::is_syncword_mp123(const byte* ptr) {
    static const char abl2[16] = { 0, 7, 7, 7, 0, 7, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8 };
    if((ptr[0] & 0xFF) != 0xFF) return false;
    if((ptr[1] & 0xE0) != 0xE0) return false;
    if((ptr[1] & 0x18) == 0x08) return false;
    if((ptr[1] & 0x06) == 0x00) return false;
    if((ptr[2] & 0xF0) == 0xF0) return false;
    if((ptr[2] & 0x0C) == 0x0C) return false;
    if((ptr[1] & 0x18) == 0x18 && (ptr[1] & 0x06) == 0x04 && abl2[ptr[2] >> 4] & (1 << (ptr[3] >> 6))) return false;
    return (ptr[3] & 3) != 2;
}

mp3_format lame_2_header(const mp3data_struct& mp3data) {
    mp3_format fmt;
    fmt.bitrate = mp3data.bitrate;
    fmt.channels = mp3data.stereo;
    fmt.samplerate = mp3data.samplerate;
    fmt.duration = mp3data.totalframes / mp3data.samplerate;
    fmt.total_frames = mp3data.totalframes;
    return fmt;
}
