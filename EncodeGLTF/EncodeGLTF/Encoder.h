#ifndef ENCODER_H
#define ENCODER_H
#include <string>

namespace GLTF_ENCODER {
	enum GE_STATE {
		GES_OK,
		GES_ERR
	};

	class Encoder {
	public:
		GE_STATE EncodeFromAsciiMemory(const std::string&) const;
		std::string GetErrorMsg() const;
		std::string GetWarnMsg() const;
	private:
		// helper functions
		GE_STATE loadModel(const std::string&) const; // load model data from json data
		GE_STATE makeMesh() const; // make a Mesh object compatible with draco
	};
}
#endif // !ENCODER_H
