//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef SIMPLE_MP3_MP3_STREAM_HPP
#define SIMPLE_MP3_MP3_STREAM_HPP

#include "audio_stream.hpp"
#include "ring_buffer.hpp"
#include "../log.hpp"
#include <lame/lame.h>
#include <fstream>
#include <cassert>

using byte = unsigned char;

struct mp3_format {
    int samplerate;
    int bitrate;
    int channels;
    int total_frames;
    int duration;   // In seconds
};

constexpr size_t mp3_frame_size = 1152;

class mp3_stream : public audio_stream<short> {
public:
    mp3_stream(const std::string& filename, size_t frame_size, audio_stream<short>* parent) : filename_(filename),
            header_parsed_(false), frame_size_(frame_size), output_buffer_(100*mp3_frame_size),
            input_buffer_(100*frame_size) {
        read_buf.resize(frame_size);
    }
    virtual ~mp3_stream() { if(file_.is_open()) file_.close(); }

    bool is_open() const { return file_.is_open(); }
    const mp3_format& get_header() const { return header_; }

    bool start() {
        initialise();
        file_ = std::ifstream(filename_, std::ios_base::binary | std::ios_base::in);

        if(file_.is_open()) {
            file_.seekg(0, std::ios::end);
            filelen_ = file_.tellg();
            file_.seekg(0, std::ios::beg);

            fill_input_buffer();

            // Load some data into the input buffer and try to strip the ID3 header
            if(!strip_header()) {
                if(is_open()) file_.close();
                LOG("Could not strip header");
                return false;
            }

            while(!header_parsed_ && is_open()) {
                fill_output_buffer();
            }
        }
        return true;
    }

    virtual size_t read(buffer_t& buffer, size_t len) {
        if(output_buffer_.size() < len) fill_output_buffer();
        auto l = output_buffer_.read(buffer.data(), len);
        return l;
    }

    virtual size_t write(const buffer_t& buffer, size_t len) {
        return 0;
    }

protected:
    std::vector<byte> read_buf;

    bool initialise() {
        lame_ = lame_init();
        if(!lame_) {
            LOG("LAME failed to initialise");
            return false;
        }

        lame_set_decode_only(lame_, 1);
        lame_set_num_samples(lame_, std::numeric_limits<unsigned int>::max());

        hip_ = hip_decode_init();
        if(!hip_) {
            LOG("LAME HIP failed to initialise");
            return false;
        }
        return true;
    }

    void shutdown() {
        hip_decode_exit(hip_);
        hip_ = nullptr;
        lame_close(lame_);
        lame_ = nullptr;
    }

    void fill_input_buffer() {
        while(is_open() && (input_buffer_.size() < input_buffer_.capacity()/2)) {
            auto rd = std::min(size_t(filelen_ - file_.tellg()), frame_size_);
            file_.read(reinterpret_cast<char*>(read_buf.data()), frame_size_);
            size_t off = file_.tellg();
            if(off == -1) file_.close();
            else          input_buffer_.write(read_buf.data(), rd);
        }
    }

    int zip(short* left_pcm, short* right_pcm, int len) {
        for(int i = 0; i != len; ++i) {
            output_buffer_.write(left_pcm+i, 1);
            output_buffer_.write(right_pcm+i, 1);
        }
        return len;
    }

    short left_pcm[mp3_frame_size];
    short right_pcm[mp3_frame_size];

    void fill_output_buffer() {
        int ret = 0, len = 0;
        while(output_buffer_.size() < output_buffer_.capacity()/2 && input_buffer_.remaining() > 0) {
            len = input_buffer_.read(read_buf.data(), frame_size_);
            if(!header_parsed_) {
                ret = hip_decode1_headers(hip_, read_buf.data(), len, left_pcm, right_pcm, &mp3data_);
                if(mp3data_.header_parsed) header_parsed_ = true;
            } else {
                ret = hip_decode1_headers(hip_, read_buf.data(), len, left_pcm, right_pcm, &mp3data_);
                if(ret > 0) zip(left_pcm, right_pcm, ret);
            }
            fill_input_buffer();
        }
    }

    bool strip_header() {
        assert(input_buffer_.read_position() == 0 && "buffer should be positioned at zero");
        byte header[100];
        if(input_buffer_.read(header, 4) != 4) return false;

        if(memcmp(header, "ID3", 3) == 0) { // Check ID3 Header
            LOG("ID3 found");
            // Read the length of the id3 header
            if(input_buffer_.read(header, 6) != 6) return false;
            header[2] &= 0x7f; header[3] &= 0x7f; header[4] &= 0x7f; header[5] &= 0x7f;
            size_t skip = (((((header[2] << 7) + header[3]) << 7) + header[4] ) << 7) + header[5];
            LOG("skipping =", skip);
            if(input_buffer_.skip(skip) != skip) return false;
            if(input_buffer_.read(header, 4) != 4) return false;   // Reposition buffer
        }

        if(memcmp(header, "AiD\1", 4) == 0) { // Check for Album ID Header
            LOG("Album ID found");
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
                LOG("Corrupted file, more than 2048 bytes skipped after headers and still no sync word");
                return false;
            }
        }
        return true;
    }

    bool is_syncword_mp123(const byte* ptr) {
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

private:
    std::string filename_;
    bool header_parsed_;
    size_t filelen_;
    size_t frame_size_;
    ring_buffer<short> output_buffer_;
    ring_buffer<byte> input_buffer_;
    mp3_format header_;
    std::ifstream file_;
    lame_t lame_;
    hip_t hip_;
    mp3data_struct mp3data_;
};

#endif //SIMPLE_MP3_MP3_STREAM_HPP
