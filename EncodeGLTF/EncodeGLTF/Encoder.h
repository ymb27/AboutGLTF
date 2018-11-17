#ifndef ENCODER_H
#define ENCODER_H
#include <string>

namespace GLTF_ENCODER {
	enum GE_STATE {
		GES_OK
	};

	class Encoder {
	public:
		GE_STATE EncodeFromAsciiMemory(std::string);
	private:
	};
}
#endif // !ENCODER_H
