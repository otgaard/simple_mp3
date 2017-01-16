//
// Created by Darren Otgaar on 2016/01/28.
//

#ifndef SIMPLE_MP3_DECODER_H
#define SIMPLE_MP3_DECODER_H

#include <vector>
#include <algorithm>
#include <iterator>
#ifdef _WIN32
#include <lame.h>
#else
#include <lame/lame.h>
#endif //_WIN32
#include "streams/mp3_stream.hpp"
#include "block_buffer.hpp"

class file_decoder {
public:
    bool initialise();
    void shutdown();

    block_buffer<short> decode_file(const std::string& filename);

private:
    lame_t lame_;
    hip_t hip_;
    mp3_format format_;
};

#endif //SIMPLE_MP3_DECODER_H
