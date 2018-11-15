#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling
#include <tinygltf/tiny_gltf.h>

#include "Encoder.h"
#include <iostream>
using std::cout;
using std::endl;
using namespace GLTF_ENCODER;

GE_STATE Encoder::EncodeFromAsciiMemory(std::string str) {
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	std::string base_dir;
	bool ret = loader.LoadASCIIFromString(&model, &err, &warn, str.c_str(), str.size(), base_dir);
	return GES_OK;
}