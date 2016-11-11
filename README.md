# zapAudio

A set of C++ classes for working with the libmp3lame and portaudio libraries.

Example player at bin/simple_mp3

# Build Instructions

(portaudio & LAME installed in third_party directory)

Build portaudio
---------------

cd portaudio

mkdir build_

cd build_

cmake .. -DCMAKE_INSTALL_PREFIX:PATH=${SIMPLE_MP3_SOURCE_PATH}/third_party && make all install

Build LAME
----------

cd lame

./configure --prefix=${SIMPLE_MP3_SOURCE_PATH}/third_party/ && make all install

Building zapAudio
-----------------

mkdir build

cd build

cmake .. -DCMAKE_INSTALL_PREFIX:PATH=${INSTALLATION_PATH} && make install
