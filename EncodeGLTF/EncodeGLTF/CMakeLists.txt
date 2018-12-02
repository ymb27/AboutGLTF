cmake_minimum_required(VERSION 3.0)

PROJECT (EncodeGLTF)

set(DRACO_LIB_PATH)

INCLUDE_DIRECTORIES(
    ../3rdpart/draco/include
    ../3rdpart/tinygltf/include
)
LINK_DIRECTORIES(
    # ../3rdpart/draco/lib
    ${DRACO_LIB_PATH}
)

add_library(EncodeGLTF Encoder.cpp)
target_link_libraries(EncodeGLTF draco dracoenc)
