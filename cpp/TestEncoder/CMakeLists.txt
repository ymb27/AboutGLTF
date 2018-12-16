include_directories(
    "${PROJECT_SOURCE_DIR}/EncodeGLTF"
    "${PROJECT_SOURCE_DIR}/3rdpart/zlib/include")
link_directories(
    ${PROJECT_BINARY_DIR}/lib)
message(STATUS ${PROJECT_SOURCE_DIR})
ADD_EXECUTABLE (testEncoder main.cpp)

target_link_libraries(testEncoder ${DRACOLIB} ${ZLIB} EncodeGLTF)
message(STATUS ${LINK_LIBRARIES})
