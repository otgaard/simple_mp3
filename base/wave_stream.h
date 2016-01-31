//
// Created by Darren Otgaar on 2016/01/31.
//

#ifndef SIMPLE_MP3_WAVE_STREAM_H
#define SIMPLE_MP3_WAVE_STREAM_H

#include <cassert>
#include <string>
#include <fstream>
#include "audio_stream.h"

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

class wave_stream : public audio_stream<short> {
public:
    wave_stream(const std::string& filename, size_t frame_size, audio_stream<short>* parent) : filename_(filename),
        frame_size_(frame_size), audio_stream<short>(parent) { buffer_.resize(8*frame_size_); }
    virtual ~wave_stream() { }

    bool is_open() const { return file_.is_open(); }

    const WAVE& get_header() const { return header_; }

    bool start() {
        file_ = std::ifstream(filename_, std::ios_base::binary | std::ios_base::in);
        if(file_.is_open()) {
            read_offset_ = 0;
            if(!read_header(header_) || !header_.is_valid()) {
                file_.close();
                return false;
            }
            return fill_buffer();
        }
        return true;
    }

    virtual size_t read(buffer_t& buffer, size_t len) {
        assert(buffer.size() >= len);
        if(len != frame_size_) return 0;    // Must be aligned
        auto l = std::min(len, buffer_end_ - read_offset_);
        if(l == 0) { file_.close(); return 0; }
        std::copy(buffer_.begin()+read_offset_, buffer_.begin()+read_offset_+l, buffer.begin());
        read_offset_ += l;
        if(read_offset_ == buffer_end_) fill_buffer();
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
            auto s = file_.tellg();
            // TODO: Fix final buffer handling.
            file_.read(reinterpret_cast<char*>(buffer_.data()), 8*frame_size_*sizeof(short));
            auto e = file_.tellg();
            if(e == -1) {
                read_offset_ = buffer_end_ = 0;
                return false;
            }
            buffer_end_ = (e - s)/sizeof(short);
            read_offset_ = 0;
            return true;
        }
        read_offset_ = buffer_end_ = 0;
        return false;
    }

private:
    std::string filename_;
    size_t frame_size_;
    std::vector<short> buffer_;
    WAVE header_;
    size_t read_offset_;
    size_t buffer_end_;
    std::ifstream file_;
};

#endif //SIMPLE_MP3_WAVE_STREAM_H
