//
// Created by Darren Otgaar on 2016/01/28.
//

#include <fstream>
#include "../log.hpp"
#include "file_decoder.hpp"
#include <cassert>
#ifdef _WIN32
#include <lame.h>
#else
#include <lame/lame.h>
#endif //_WIN32
#include <cstring>

using byte = unsigned char;

bool strip_header(block_buffer<byte>& buffer);
bool is_syncword_mp123(const byte* ptr);

bool file_decoder::initialise() {
    lame_ = lame_init();
    if(!lame_) {
        SM_LOG("LAME failed to initialise");
        return false;
    }

    lame_set_decode_only(lame_, 1);
    lame_set_num_samples(lame_, std::numeric_limits<unsigned int>::max());

    hip_ = hip_decode_init();
    if(!hip_) {
        SM_LOG("LAME HIP failed to initialise");
        return false;
    }
    return true;
}

void file_decoder::shutdown() {
    hip_decode_exit(hip_);
    hip_ = nullptr;
    lame_close(lame_);
    lame_ = nullptr;
}

int zip(block_buffer<short>& buffer, short* left_pcm, short* right_pcm, int len) {
    for(int i = 0; i != len; ++i) {
        buffer.write(left_pcm+i, 1);
        buffer.write(right_pcm+i, 1);
    }
    return len;
}

block_buffer<short> file_decoder::decode_file(const std::string& filename) {
    block_buffer<byte> file_contents;
    block_buffer<short> sample_buffer;

    std::ifstream file;
    file.open(filename, std::ios_base::binary | std::ios_base::in);
    if(!file.is_open()) { SM_LOG("Error opening file"); return sample_buffer; }
    file.seekg(0, std::ios_base::end);
    auto file_len = file.tellg();
    file.seekg(0, std::ios_base::beg);
    assert(file.tellg() == std::streamoff(0));
	std::vector<byte> buffer((size_t)file_len);
    file.read((char*)(buffer.data()), file_len);
    file.close();

    file_contents.write(buffer.data(), (size_t)file_len);
    SM_LOG("File Loaded, size =", file_contents.size() / (1000000.f), "MB");

    // First, we need to skip the id3 header and position the file on the start of the mp3 header stream.  We therefore
    // need to scan the file for the sync word.

    SM_LOG("STRIPPING HEADER: ", strip_header(file_contents));

    byte file_block[1024];
    short left_pcm[1152], right_pcm[1152];
    mp3data_struct mp3data;
    memset(&mp3data, 0x00, sizeof(mp3data_struct));
    bool header_parsed = false;

    int ret = 0, len = 0;
    while((len = file_contents.read(file_block, 1024)) > 0) {
        if(!header_parsed) {
            ret = hip_decode1_headers(hip_, file_block, len, left_pcm, right_pcm, &mp3data);
            if(mp3data.header_parsed == 1) {
                SM_LOG("Header Parsed", ret);
                SM_LOG("bitrate =", mp3data.bitrate);
                SM_LOG("channels =", mp3data.stereo);
                SM_LOG("samplerate =", mp3data.samplerate);
                format_.samplerate = mp3data.samplerate;
                format_.bitrate = mp3data.bitrate;
                format_.channels = mp3data.stereo;
                format_.total_frames = mp3data.totalframes;
                format_.duration = mp3data.totalframes / format_.samplerate;
                SM_LOG("duration =", format_.duration);
                header_parsed = true;
            }
        } else {
            ret = hip_decode1_headers(hip_, file_block, len, left_pcm, right_pcm, &mp3data);
            zip(sample_buffer, left_pcm, right_pcm, ret);
            //sample_buffer.write(left_pcm, ret);
        }
    }

    return sample_buffer;
}

bool strip_header(block_buffer<byte>& buffer) {
    assert(buffer.get_cursor() == 0 && "buffer should be positioned at zero");
    byte header[100];
    if(buffer.read(header, 4) != 4) return false;

    if(memcmp(header, "ID3", 3) == 0) { // Check ID3 Header
        SM_LOG("ID3 found");
        // Read the length of the id3 header
        if(buffer.read(header, 6) != 6) return false;
        header[2] &= 0x7f; header[3] &= 0x7f; header[4] &= 0x7f; header[5] &= 0x7f;
        size_t skip = (((((header[2] << 7) + header[3]) << 7) + header[4] ) << 7) + header[5];
        if(buffer.skip(skip) != skip) return false;
        if(buffer.read(header, 4) != 4) return false;   // Reposition buffer
    }

    if(memcmp(header, "AiD\1", 4) == 0) { // Check for Album ID Header
        SM_LOG("Album ID found");
        if(buffer.read(header, 2) != 2) return false;
        size_t skip = (size_t)header[0] + 256 * (size_t)header[1];
        if(buffer.skip(skip - 6) != (skip - 6)) return false;
        if(buffer.read(header, 4) != 4) return false;   // Reposition buffer
    }

    // Now scan to find the mp3 sync word
    size_t skipped = 0;
    while(!is_syncword_mp123(buffer.read_ptr())) {
        byte dummy;
        if(buffer.read(&dummy, 1) != 1) return false;
        skipped++;
        if(skipped >= 2048) {
            SM_LOG("Corrupted file, more than 2048 bytes skipped after headers and still no sync word");
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
