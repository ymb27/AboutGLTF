#pragma once
#include <string>

namespace GLTF_ENCODER {
	enum GE_STATE {
		GES_OK
	};

	class Encoder {
	public:
		GE_STATE EncodeFromAsciiMemory(std::string);
	private:
		void* 
	};
}
