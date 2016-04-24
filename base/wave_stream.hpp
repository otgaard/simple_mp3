//
// Created by Darren Otgaar on 2016/01/31.
//

#ifndef SIMPLE_MP3_WAVE_STREAM_HPP
#define SIMPLE_MP3_WAVE_STREAM_HPP

#include <cassert>
#include <string>
#include <fstream>
#include "audio_stream.hpp"
#include "ring_buffer.hpp"

struct WAVE {
    char chunk_id[4];
    uint32_t chunk_size;
    char format_id[4];
    char chunk1_id[4];
    uint32_t chunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char chunk2_id[4];
    uint32_t chunk2_size;

    bool is_valid() const {
        return ((strncmp(chunk_id, "RIFF", 4) == 0) &&
                (strncmp(format_id, "WAVE", 4) == 0) &&
                (strncmp(chunk1_id, "fmt ", 4) == 0) &&
                (strncmp(chunk2_id, "data", 4) == 0));
    }
};

constexpr size_t buffer_scale = 100;
constexpr size_t buffer_fetch = 50;
constexpr size_t buffer_update = 50;

class wave_stream : public audio_stream<short> {
public:
    wave_stream(const std::string& filename, size_t frame_size, audio_stream<short>* parent) : audio_stream<short>(parent),
        filename_(filename), frame_size_(frame_size), buffer_(buffer_scale*frame_size_) { }
    virtual ~wave_stream() { if(file_.is_open()) file_.close(); }

    bool is_open() const { return file_.is_open(); }

    const WAVE& get_header() const { return header_; }

    bool start() {
        file_ = std::ifstream(filename_, std::ios_base::binary | std::ios_base::in);

        if(file_.is_open()) {
            file_.seekg(0, std::ios::end);
            file_length_ = file_.tellg();
            file_.seekg(0, std::ios::beg);
            if(!read_header(header_) || !header_.is_valid()) {
                file_.close();
                return false;
            }
            return fill_buffer();
        }
        return false;
    }

    virtual size_t read(buffer_t& buffer, size_t len) {
        auto l = buffer_.read(buffer.data(), len);
        if(buffer_.size() < buffer_update*frame_size_) fill_buffer();
        return l;
    }

    virtual size_t write(const buffer_t& buffer, size_t len) {
        return 0;
    }

protected:
    bool read_header(WAVE& header) {
        if(file_.is_open() && file_.tellg() == 0) {
            file_.read(reinterpret_cast<char*>(&header), sizeof(WAVE));
            return file_.tellg() == sizeof(WAVE);
        }
        return false;
    }

    bool fill_buffer() {
        if(file_.is_open() && !file_.eof()) {
            if(buffer_.remaining() < buffer_update*frame_size_) return false;
            auto rd = std::min(size_t(file_length_ - file_.tellg()), buffer_fetch*frame_size_);
            std::vector<short> arr(rd);
            file_.read(reinterpret_cast<char*>(arr.data()), rd*sizeof(short));
            buffer_.write(arr.data(), rd);
            auto e = file_.tellg();
            if(e == -1) file_.close();
            return true;
        }
        return false;
    }

private:
    std::string filename_;
    size_t file_length_;
    size_t frame_size_;
    ring_buffer<short> buffer_;
    WAVE header_;
    std::ifstream file_;
};

#endif //SIMPLE_MP3_WAVE_STREAM_HPP
