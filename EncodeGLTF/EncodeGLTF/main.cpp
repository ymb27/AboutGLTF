#include "Encoder.h"
#include "Helper.h"
#include <iostream>

const char* FILE_PATH = "../../sample.gltf";

int main() {
	GLTF_ENCODER::Encoder ec;
	ec.EncodeFromAsciiMemory(
		LoadStringFromFile(FILE_PATH)
	);
	system("pause");
	return 0;
}