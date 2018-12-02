include_directories(../EncodeGLTF)
link_directories(../build/lib ../3rdpart/draco/lib)
link_libraries(EncodeGLTF dracoenc)

ADD_EXECUTABLE (testEncoder main.cpp)