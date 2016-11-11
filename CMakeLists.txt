cmake_minimum_required(VERSION 3.3)
project(simple_mp3)

# Build and Install Portaudio in the third_party directory
find_package(portaudio REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)
find_package(lame REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)

set(CMAKE_CXX_STANDARD 14)

set(SIMPLEMP3_PUB_HEADERS
    decoder/decoder.hpp
    base/ring_buffer.hpp
    base/mp3_stream.hpp
    base/audio_stream.hpp
    base/wave_stream.hpp
    log.hpp
)

set(SIMPLEMP3_FILES decoder/decoder.cpp ${SIMPLEMP3_PUB_HEADERS})
add_library(simplemp3 SHARED ${SIMPLEMP3_FILES})
target_include_directories(simplemp3 PUBLIC ${lame_INCLUDE_DIRS} ${portaudio_INCLUDE_DIRS})
target_link_libraries(simplemp3 ${lame_LIBRARIES} ${portaudio_LIBRARIES})

set(SOURCE_FILES  main.cpp )
add_executable(simple_mp3 ${SOURCE_FILES})
target_include_directories(simple_mp3 PUBLIC ${lame_INCLUDE_DIRS})
target_link_libraries(simple_mp3 ${lame_LIBRARIES} ${portaudio_LIBRARIES} simplemp3)

install(TARGETS simplemp3 DESTINATION lib)
install(TARGETS simple_mp3 DESTINATION bin)

foreach(file ${SIMPLEMP3_PUB_HEADERS})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION include/${dir})
endforeach()
