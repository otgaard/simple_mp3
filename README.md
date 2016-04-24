# simple_mp3
A set of C++ classes for working with the libmp3lame and portaudio libraries.

A simple whole file decoder is in decoder/decoder.hpp and a ring-buffer stream 
base is in base/mp3_stream.hpp.  Note that the file decoder is very inefficient 
decodes the whole file at once.  The mp3_stream is much better suited to being
used in an actual application.

