cmake_minimum_required(VERSION 2.6)
project(noise)

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-lprotobuf -lrocksdb -lyajl")
set(noise_VERSION_MAJOR 0)
set(noise_VERSION_MINOR 1)

include_directories(/usr/local/include)
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/noise)
link_directories(/usr/local/lib)

# https://github.com/Eyescale/CMake/issues/464
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake)
include(Findrocksdb)


find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
find_package(rocksdb REQUIRED)

find_package(Yajl)
include_directories(${YAJL_INCLUDE_DIR})



set(common_files_c
  noise/porter.c
)

set(common_files_cc
  noise/index.cpp
  noise/query.cpp
  noise/results.cpp
  noise/json_shred.cpp
  noise/stems.cpp
  noise/records.pb.cc
)

add_executable(index
  ${common_files_c}
  ${common_files_cc}
  noise/main.cpp
)

add_executable(query
  ${common_files_c}
  ${common_files_cc}
  test/main.cpp
)
