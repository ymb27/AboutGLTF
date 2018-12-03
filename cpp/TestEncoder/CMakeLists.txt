include_directories("${PROJECT_SOURCE_DIR}/EncodeGLTF")
link_directories(
    ${PROJECT_BINARY_DIR}/lib)
message(STATUS ${PROJECT_SOURCE_DIR})
ADD_EXECUTABLE (testEncoder main.cpp)
target_link_libraries(testEncoder ${DRACOLIB})
target_link_libraries(testEncoder EncodeGLTF)