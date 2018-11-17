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

// 全局变量
struct {
	tinygltf::Model model;
	std::string err;
	std::string warn;
} GVAR;

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) const {
	GE_STATE state;
	state = loadModel(jData);
	if (state != GES_OK) return state;
}

std::string Encoder::GetErrorMsg() const { return GVAR.err; }
std::string Encoder::GetWarnMsg() const { return GVAR.warn; }

// private helper functions
GE_STATE Encoder::loadModel(const std::string& jData) const {
	std::string base_dir;
	tinygltf::TinyGLTF loader;
	bool ret = loader.LoadASCIIFromString(&GVAR.model, &GVAR.err, &GVAR.warn, jData.c_str(), jData.size(), base_dir);
	if (ret) return GES_OK;
	else {
		// 需要根据错误信息(err)判定返回的类型
		return GES_ERR;
	}
}

GE_STATE Encoder::makeMesh() const {

}
