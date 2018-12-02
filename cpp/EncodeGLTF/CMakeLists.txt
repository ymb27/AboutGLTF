INCLUDE_DIRECTORIES(
    ../3rdpart/draco/include
    ../3rdpart/tinygltf/include
)
LINK_DIRECTORIES(
    ../3rdpart/draco/lib
)
LINK_LIBRARIES(dracoenc)

add_library(EncodeGLTF Encoder.cpp)
