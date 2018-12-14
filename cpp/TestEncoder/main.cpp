#include <Encoder.h>
#include <zlib_wrapper.hpp>
#include <iostream>
#include <fstream>
#include <assert.h>

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
	template<typename T>
	std::vector<T> data() {
		std::vector<T> ret;
		if (!file.is_open()) return ret;
		file.seekg(0, std::ios::end);
		size_t fileSize = static_cast<size_t>(file.tellg());
		file.seekg(0, std::ios::beg);

		ret.resize(fileSize);
		file.read(reinterpret_cast<char*>(ret.data()), fileSize);
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
	std::vector<char> chars = ifc.data<char>();
	std::string ret = std::string(&chars[0], chars.size());
	return ret;
}
/* refer to TinyGltf */
static std::string GetBaseDir(const std::string &filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos) {
		return filepath.substr(0, filepath.find_last_of("/\\"));
	}
	return "./";
}

static std::string GetFileName(const std::string& filePath) {
	size_t dot = filePath.find_last_of(".");
	size_t slash = filePath.find_last_of("/\\");
	if (slash != std::string::npos) {
		return filePath.substr(slash + 1, dot - slash - 1);
	}
	return filePath.substr(0, dot);
}

void encode(const char* path, const char* outputName = nullptr) {
	GLTF_ENCODER::Encoder ec;
	GLTF_ENCODER::GE_STATE state;
	std::string baseDir = GetBaseDir(path);
	ec.SetBaseDir(baseDir);
	state = ec.EncodeFromAsciiMemory(
		LoadStringFromFile(path)
	);
	if (state != GLTF_ENCODER::GES_OK) {
		std::cout << ec.ErrorMsg() << std::endl;
	}
	else {
		std::unique_ptr<std::vector<uint8_t> > compressedData = ec.Buffer();
		std::ofstream outputFile;
		baseDir = baseDir + "\\";
		if (outputName) baseDir += outputName;
		else baseDir += "test";
		baseDir += ".zglb";
		outputFile.open(baseDir.c_str(), std::ios::binary | std::ios::trunc | std::ios::out);
		if (!outputFile.is_open()) {
			std::cout << "open outputFile failed!" << std::endl;
			return;
		}
		outputFile.write(reinterpret_cast<char*>(compressedData->data()), compressedData->size());
		outputFile.close();
	}
}

void decode(const char* path) {
	InputFileContainer ifile(path, std::ios::in | std::ios::binary);
	if (!ifile.state()) {
		std::cout << "open file failed" << std::endl;
		return;
	}
	std::vector<uint8_t>&& fileData = ifile.data<uint8_t>();
	uint8_t* fileDataPtr = reinterpret_cast<uint8_t*>(fileData.data());
	GLTF_ENCODER::CompressHeader compressHeader;
	memcpy(&compressHeader, fileDataPtr, sizeof(compressHeader)); fileDataPtr += sizeof(compressHeader);
	std::vector<uint8_t> srcData(compressHeader.cprSize);
	std::vector<uint8_t> jsonData(compressHeader.glbjsonSize);
	std::vector<uint8_t> glbData;
	glbData.resize(compressHeader.glbHeaderSize + jsonData.size() + compressHeader.glbBinSize);
	uint8_t* glbDataPtr = glbData.data();

	memcpy(glbDataPtr, fileDataPtr, compressHeader.glbHeaderSize);
	fileDataPtr += compressHeader.glbHeaderSize; glbDataPtr += compressHeader.glbHeaderSize;

	memcpy(srcData.data(), fileDataPtr, compressHeader.cprSize); fileDataPtr += compressHeader.cprSize;
	zlib_wrapper::uncompress(jsonData, srcData);
	memcpy(glbDataPtr, jsonData.data(), compressHeader.glbjsonSize); glbDataPtr += compressHeader.glbjsonSize;

	memcpy(glbDataPtr, fileDataPtr, compressHeader.glbBinSize);

	std::ofstream glbFile;
	std::string outputPath = GetBaseDir(path) + "/" + GetFileName(path) + ".glb";
	glbFile.open(outputPath, std::ios::out | std::ios::binary | std::ios::trunc);
	if (!glbFile.is_open()) {
		std::cout << "open output file failed!" << std::endl;
		return;
	}
	glbFile.write(reinterpret_cast<const char*>(glbData.data()), glbData.size());
	glbFile.close();
}

int main(int argc, char* argv[]) {
	if (argc <= 2) {
		std::cout << "invalid number of parameters" << std::endl;
		return 0;
	}
	else {
		/* decode */
		if (!std::strcmp(argv[1], "-D")) {
			decode(argv[2]);
		}
		/* encode */
		else if (!std::strcmp(argv[1], "-E")) {
			if (argc == 4) encode(argv[2], argv[3]);
			else encode(argv[2]);
		}
		else if (!std::strcmp(argv[1], "-ED")) {
			encode(argv[2]);
			std::string filePath = GetBaseDir(argv[2]);
			filePath += "/test.zglb";
			decode(filePath.c_str());
		}
		else
			std::cout << "invalid parameter " << argv[1] << std::endl;
	}
	return 0;
}