#include "Encoder.h"
#include "Helper.h"
#include <iostream>
#include "Utility.h"
const char* FILE_PATH = "./ball.gltf";

int main() {
	GLTF_ENCODER::Encoder ec;
	GLTF_ENCODER::GE_STATE state;
	state = ec.EncodeFromAsciiMemory(
		LoadStringFromFile(FILE_PATH)
	);
	if (state != GLTF_ENCODER::GES_OK)
		std::cout << ec.GetErrorMsg() << std::endl;
	system("pause");
	return 0;
}