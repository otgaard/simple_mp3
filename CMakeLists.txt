cmake_minimum_required(VERSION 3.3)
project(simple_mp3)

set(CMAKE_BUILD_TYPE "Debug")

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/output/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/output/bin)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(APPLE) # Should contain lame and portaudio
set(OPTIONAL_INCLUDES /Users/otgaard/opt)
elseif(WIN32)
set(OPTIONAL_INCLUDES C://opt)
endif(APPLE)

set(SIMPLEMP3_FILES decoder/decoder.cpp decoder/decoder.h)
add_library(simplemp3 ${SIMPLEMP3_FILES})
target_include_directories(simplemp3 PUBLIC ${OPTIONAL_INCLUDES}/lame)

set(SOURCE_FILES base/audio_stream.h main.cpp base/wave_stream.h log.hpp)
add_executable(simple_mp3 ${SOURCE_FILES})
target_include_directories(simple_mp3 PUBLIC ${OPTIONAL_INCLUDES}/lame)
target_link_libraries(simple_mp3 ${OPTIONAL_INCLUDES}/lame/lib/libmp3lame.a portaudio simplemp3)
