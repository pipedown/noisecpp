# Noise

Nested Object Inverted Search Engine

This is a native library meant to be linked into other projects and will expose a C API.

It's a full text index and query system that understands the semi structured natured of JSON, and will allow:
* Stemmed word/fuzzy match
* Case sensive Exact word and sentence match
* Arbitrary boolean nesting
* Greater Than/Less Than matching


## License
Apache 2.0


## Build dependencies:
* C++ compiler, rustc (1.8 nightlies), cmake, rocksdb, protobuf, yajl. On OS/X all dependencies

## To build in Xcode:
* mkdir build && cd build && cmake -G Xcode .. && open noise.xcodeproj

## To build for Unix Makefiles:
* mkdir build && cd build && cmake .. && make -j5

## To test:
* make test
* ctest

## HTTP commands:
* HAHA I make joke.

## Troubleshooting:
* HAHA again!!
