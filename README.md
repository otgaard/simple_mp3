# simple_mp3
A set of C++ classes for working with the libmp3lame and portaudio libraries.

A simple whole file decoder is in decoder/decoder.hpp and a ring-buffer stream 
base is in base/mp3_stream.hpp.  Note that the file decoder is very inefficient 
and decodes the whole file at once.  The mp3_stream is much better suited to being
used in an actual application.

# Build Instructions

(portaudio & LAME installed in third_party directory)

Build portaudio
---------------

cd portaudio

mkdir build_

cd build_

cmake .. -DCMAKE_INSTALL_PREFIX=PATH:${SIMPLE_MP3_SOURCE_PATH}/third_party && make all install

Build LAME
----------

cd lame

./configure --prefix=${SIMPLE_MP3_SOURCE_PATH}/third_party/ && make all install

Building simple_mp3
-------------------

mkdir build

cd build

cmake .. -DCMAKE_INSTALL_PREFIX=PATH:${INSTALLATION_PATH} && make install
