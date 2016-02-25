cmake_minimum_required(VERSION 2.6)
project(noise)

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS "-std=gnu++11 -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "-lprotobuf -lrocksdb -lyajl")
set(noise_VERSION_MAJOR 0)
set(noise_VERSION_MINOR 1)

include_directories(/usr/local/include)
include_directories(BEFORE ${PROJECT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src)
link_directories(/usr/local/lib)
add_subdirectory(src/proto)
#add_subdirectory(proto)



# https://github.com/Eyescale/CMake/issues/464
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake
                              ${CMAKE_SOURCE_DIR}/CMake)
include(Findrocksdb)
find_package(rocksdb REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Yajl REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${YAJL_INCLUDE_DIR})

#set(ProtoFiles records.proto)
#protobuf_generate_cpp(ProtoSources ProtoHeaders ${ProtoFiles})

#protobuf_generate_cpp(ProtoSources ProtoHeaders records.proto)

set(common_files_c
  src/porter.c
)

set(common_files_h
  src/json_shred.h
  src/key_builder.h
  src/noise.h
  src/porter.h
  src/query.h
  src/results.h
  src/stems.h
)

set(common_files_cc
  src/index.cpp
  src/query.cpp
  src/results.cpp
  src/json_shred.cpp
  src/stems.cpp
)

add_executable(index
  ${common_files_h}
  ${common_files_c}
  ${common_files_cc}
  ${ProtoSources}
  ${ProtoHeaders}
  src/index_main.cpp
)

add_executable(query
  ${common_files_h}
  ${common_files_c}
  ${common_files_cc}
  ${ProtoSources}
  ${ProtoHeaders}
  src/query_main.cpp
)

target_link_libraries(index proto ${PROTOBUF_LIBRARY})
target_link_libraries(query proto ${PROTOBUF_LIBRARY})
