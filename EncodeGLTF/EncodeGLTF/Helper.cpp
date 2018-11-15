#include "Helper.h"
#include <fstream>
#include <assert.h>
#include <iostream>
using std::cout;
using std::endl;

class File {
public:
	File(std::string path, unsigned int method)
		:f(std::ifstream(path.c_str(), method)) {}
	~File() {
		if (f.is_open()) f.close();
	}
	std::ifstream f;
	std::ifstream& operator()(void) { return f; }
};

std::string LoadStringFromFile(std::string path) {
	File file(path, std::ios::in);
	assert(file().is_open());
	// Get file size
	file().seekg(0, std::ios::end);
	size_t fileSize = file().tellg();
	file().seekg(0, std::ios::beg);
	
	char* container = new char[fileSize];
	memset(container, 0, fileSize);
	file().read(container, fileSize);

	std::string ret(container, fileSize);
	delete[] container;
	return ret;
}