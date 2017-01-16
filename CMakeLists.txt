cmake_minimum_required(VERSION 3.3)
project(zapAudio)

# Build and Install Portaudio in the third_party directory
if(APPLE OR UNIX)
	find_package(portaudio REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)
	find_package(lame REQUIRED PATHS ${CMAKE_SOURCE_DIR}/third_party NO_DEFAULT_PATH)
else(WIN32)
	include(GenerateExportHeader)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS)
	set(lame_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/lame-3.99.5/include)
	set(portaudio_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/portaudio/include)
	set(lame_LIBRARIES
        ${CMAKE_SOURCE_DIR}/third_party/lame-3.99.5/output/Release/libmpghip-static.lib
        ${CMAKE_SOURCE_DIR}/third_party/lame-3.99.5/output/Release/libmp3lame-static.lib)
	set(portaudio_LIBRARIES
        ${CMAKE_SOURCE_DIR}/third_party/portaudio/build_/Release/portaudio_static_x86.lib)
endif(APPLE OR UNIX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(ZAPAUDIO_PUB_HEADERS
        tools/file_decoder.hpp
        ring_buffer.hpp
        streams/mp3_stream.hpp
        streams/audio_stream.hpp
        streams/wave_stream.hpp
        audio_output.hpp
        streams/buffered_stream.hpp
        streams/sine_wave.hpp
        block_buffer.hpp
        streams/adapter_stream.hpp)

set(ZAPAUDIO_SOURCE
        streams/mp3_stream.cpp
        tools/file_decoder.cpp
        audio_output.cpp
        streams/buffered_stream.cpp
        log.hpp)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(ZAPAUDIO_FILES ${ZAPAUDIO_SOURCE} ${ZAPAUDIO_PUB_HEADERS})

if(APPLE OR UNIX)
	add_library(zapAudio SHARED ${ZAPAUDIO_FILES})
elseif(WIN32)
	add_library(zapAudio SHARED ${ZAPAUDIO_FILES} zapAudio_exports.h)
endif(APPLE OR UNIX)

target_include_directories(zapAudio PUBLIC ${lame_INCLUDE_DIRS} ${portaudio_INCLUDE_DIRS})
target_link_libraries(zapAudio ${lame_LIBRARIES} ${portaudio_LIBRARIES})

set(SOURCE_FILES simple_mp3.cpp)
add_executable(simple_mp3 ${SOURCE_FILES})
target_include_directories(simple_mp3 PUBLIC ${lame_INCLUDE_DIRS})
target_link_libraries(simple_mp3 zapAudio ${lame_LIBRARIES} ${portaudio_LIBRARIES})

if(APPLE OR UNIX)
	install(TARGETS zapAudio LIBRARY DESTINATION lib)
	install(TARGETS simple_mp3 RUNTIME DESTINATION bin)
elseif(WIN32)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	GENERATE_EXPORT_HEADER(zapAudio
			BASE_NAME zapAudio
			EXPORT_MACRO_NAME ZAPAUDIO_EXPORT
			EXPORT_FILE_NAME zapAudio_exports.h
			STATIC_DEFINE SHARED_EXPORTS_BUILT_AS_STATIC)

	install(TARGETS zapAudio DESTINATION lib)
	install(TARGETS simple_mp3 DESTINATION bin)
endif(APPLE OR UNIX)

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
