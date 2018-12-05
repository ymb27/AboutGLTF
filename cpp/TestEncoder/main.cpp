﻿#include <Encoder.h>
#include <iostream>
#include <fstream>
#include <assert.h>

std::string GBaseDir;
class InputFileContainer {
public:
	InputFileContainer(const char* name, int mode = 1) {
		file.open(name, static_cast<std::ios::openmode>(mode));
	}
	~InputFileContainer() {
		if (file.is_open()) file.close();
	}
	const bool state() const { return file.is_open(); }
	const bool isEOF() const { return  file.eof(); }
	std::vector<char> data() {
		std::vector<char> ret;
		if (!file.is_open()) return ret;
		file.seekg(0, std::ios::end);
		size_t fileSize = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		ret.resize(fileSize);
		file.read(reinterpret_cast<char*>(&ret[0]), fileSize);
		return ret;
	}
	void close() { if (file.is_open()) file.close(); }
	template<typename T>
	std::ifstream& operator>>(T& value) {
		if (!file.is_open()) abort();
		file >> value;
		return file;
	}
private:
	std::ifstream file;
};

std::string LoadStringFromFile(std::string path) {
	InputFileContainer ifc(path.c_str(), std::ios::binary);
	assert(ifc.state());
	// Get file size
	std::vector<char> chars = ifc.data();
	std::string ret = std::string(&chars[0], chars.size());
	return ret;
}
/* refer to TinyGltf */
static void GetBaseDir(const std::string &filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos) {
		GBaseDir = filepath.substr(0, filepath.find_last_of("/\\"));
		return;
	}
	GBaseDir = ".\\";
}
int main(int argc, char* argv[]) {
	GLTF_ENCODER::Encoder ec;
	GLTF_ENCODER::GE_STATE state;
	GetBaseDir(argv[1]);
	state = ec.EncodeFromAsciiMemory(
		LoadStringFromFile(argv[1])
	);
	if (state != GLTF_ENCODER::GES_OK) {
		std::cout << ec.ErrorMsg() << std::endl;
	}
	else {
		std::unique_ptr<std::vector<uint8_t> > compressedData = ec.Buffer();
		std::ofstream outputFile;
		GBaseDir += "\\test.glb";
		outputFile.open(GBaseDir.c_str(), std::ios::binary | std::ios::trunc);
		outputFile.write(reinterpret_cast<char*>(compressedData->data()), compressedData->size());
		outputFile.close();
	}
	system("pause");
	return 0;
}