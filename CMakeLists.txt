cmake_minimum_required(VERSION 3.3)
project(zapAudio)

# Build and Install Portaudio in the third_party directory
find_package(portaudio REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)
find_package(lame REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)

set(CMAKE_CXX_STANDARD 14)

set(ZAPAUDIO_PUB_HEADERS
    decoder/decoder.hpp
    base/ring_buffer.hpp
    base/mp3_stream.hpp
    base/audio_stream.hpp
    base/wave_stream.hpp
)

set(ZAPAUDIO_SOURCE
    base/mp3_stream.cpp
    decoder/decoder.cpp
    log.hpp)

set(ZAPAUDIO_FILES ${ZAPAUDIO_SOURCE} ${ZAPAUDIO_PUB_HEADERS})
add_library(zapAudio SHARED ${ZAPAUDIO_FILES})
target_include_directories(zapAudio PUBLIC ${lame_INCLUDE_DIRS} ${portaudio_INCLUDE_DIRS})
target_link_libraries(zapAudio ${lame_LIBRARIES} ${portaudio_LIBRARIES})

set(SOURCE_FILES main.cpp)
add_executable(simple_mp3 ${SOURCE_FILES})
target_include_directories(simple_mp3 PUBLIC ${lame_INCLUDE_DIRS})
target_link_libraries(simple_mp3 zapAudio ${lame_LIBRARIES} ${portaudio_LIBRARIES})

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

install(TARGETS zapAudio DESTINATION lib)
install(TARGETS simple_mp3 DESTINATION bin)

foreach(library ${portaudio_LIBRARIES})
    get_filename_component(path ${library} REALPATH)
    install(FILES ${path} DESTINATION lib)
endforeach()

foreach(library ${lame_LIBRARIES})
    get_filename_component(path ${library} REALPATH)
    install(FILES ${path} DESTINATION lib)
endforeach()

foreach(file ${ZAPAUDIO_PUB_HEADERS})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION include/zapAudio/${dir})
endforeach()

list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")
