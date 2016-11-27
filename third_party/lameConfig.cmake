set(lame_INCLUDE_DIRS ${lame_DIR}/include)
set(lame_LIBRARY_DIR ${lame_DIR}/lib)
if(APPLE)
set(lame_LIBRARIES ${lame_LIBRARIES} ${lame_LIBRARY_DIR}/libmp3lame.dylib)
else(UNIX)
set(lame_LIBRARIES ${lame_LIBRARIES} ${lame_LIBRARY_DIR}/libmp3lame.so)
endif(APPLE)

