cmake_minimum_required(VERSION 3.3)
project(simple_mp3)

# Build and Install Portaudio in the third_party directory
find_package(portaudio REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)
find_package(lame REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)

set(CMAKE_CXX_STANDARD 14)

set(SIMPLEMP3_FILES decoder/decoder.cpp decoder/decoder.hpp base/ring_buffer.hpp base/mp3_stream.hpp)
add_library(simplemp3 SHARED ${SIMPLEMP3_FILES})
target_include_directories(simplemp3 PUBLIC ${lame_INCLUDE_DIRS} ${portaudio_INCLUDE_DIRS})
target_link_libraries(simplemp3 ${lame_LIBRARIES} ${portaudio_LIBRARIES})

set(SOURCE_FILES base/audio_stream.hpp main.cpp base/wave_stream.hpp log.hpp)
add_executable(simple_mp3 ${SOURCE_FILES})
target_include_directories(simple_mp3 PUBLIC ${lame_INCLUDE_DIRS})
target_link_libraries(simple_mp3 ${lame_LIBRARIES} ${portaudio_LIBRARIES} simplemp3)
