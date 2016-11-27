set(portaudio_INCLUDE_DIRS ${portaudio_DIR}/include)
set(portaudio_LIBRARY_DIR ${portaudio_DIR}/lib)
if(APPLE)
set(portaudio_LIBRARIES ${portaudio_LIBRARIES} ${portaudio_LIBRARY_DIR}/libportaudio.dylib)
else(UNIX)
set(portaudio_LIBRARIES ${portaudio_LIBRARIES} ${portaudio_LIBRARY_DIR}/libportaudio.so)
endif(APPLE)

