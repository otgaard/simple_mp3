//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef SIMPLE_MP3_MP3_STREAM_HPP
#define SIMPLE_MP3_MP3_STREAM_HPP

#include "audio_stream.hpp"
#include "ring_buffer.hpp"
#include "../log.hpp"
#include <fstream>
#include <cassert>

struct lame_global_struct;
typedef struct lame_global_struct lame_global_flags;
typedef lame_global_flags *lame_t;

struct hip_global_struct;
typedef struct hip_global_struct hip_global_flags;
typedef hip_global_flags *hip_t;

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
    mp3_stream(const std::string& filename, size_t frame_size, audio_stream<short>* parent);
    virtual ~mp3_stream();

    bool is_open() const { return file_.is_open(); }
    const mp3_format& get_header() const { return header_; }

    bool start();

    virtual size_t read(buffer_t& buffer, size_t len);
    virtual size_t write(const buffer_t& buffer, size_t len);

protected:
    std::vector<byte> read_buf;

    bool initialise();

    void shutdown();
    void fill_input_buffer();
    int zip(short* left_pcm, short* right_pcm, int len);

    short left_pcm[mp3_frame_size];
    short right_pcm[mp3_frame_size];

    void fill_output_buffer();
    bool strip_header();
    bool is_syncword_mp123(const byte* ptr);

private:
    std::string filename_;
    bool header_parsed_;
    size_t file_size_;
    size_t frame_size_;
    ring_buffer<short> output_buffer_;
    ring_buffer<byte> input_buffer_;
    mp3_format header_;
    std::ifstream file_;
    lame_t lame_;
    hip_t hip_;
    //mp3data_struct mp3data_;
};

#endif //SIMPLE_MP3_MP3_STREAM_HPP
