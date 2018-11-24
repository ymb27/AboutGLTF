#include "Utility.h"
#include <fstream>

bool OBJHelper::Finalize(const char* file) {
	if (!state) return false;
	OutputFileContainer ofc(file);
	if (!ofc.state()) {
		err = "init file failed!";
		return false;
	}
	// name
	ofc << "o object" << std::endl;
	// vertex
	for (const OBJHelper::Position& pos : positions) {
		ofc << "v " << pos[0] << ' ' << pos[1] << ' ' << pos[2] << std::endl;
	}
	// face
	for (const OBJHelper::Face& face : faces) {
		ofc << "f " << face[0] + 1 << ' ' << face[1] + 1 << ' ' << face[2] + 1 << std::endl;
	}
	return true;
}