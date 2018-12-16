INCLUDE_DIRECTORIES(
    "${PROJECT_SOURCE_DIR}/3rdpart/draco/include"
    "${PROJECT_SOURCE_DIR}/3rdpart/tinygltf/include"
    "${PROJECT_SOURCE_DIR}/3rdpart/zlib/include"
)

add_library(EncodeGLTF Encoder.cpp)
target_link_libraries(EncodeGLTF ${DRACOLIB})
target_link_libraries(EncodeGLTF ${ZLIB})
