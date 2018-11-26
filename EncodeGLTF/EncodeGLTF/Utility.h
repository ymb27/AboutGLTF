#ifndef UTILITY_H
#define UTILITY_H
#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>
#include <draco/compression/decode.h>
#include "Encoder.h"
using GLTF_ENCODER::GE_STATE;
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <cassert>

class OutputFileContainer {
public:
	OutputFileContainer(const char* path) {
		file.open(path, std::ios::out | std::ios::trunc);
	}
	~OutputFileContainer() {
		if (file.is_open()) file.close();
	}
	const bool state() const { return file.is_open(); }
	void close() { if (file.is_open()) file.close(); }
	template<typename T>
	std::ofstream& operator<<(T value) {
		if (!file.is_open()) abort();
		file << value;
		return file;
	}
private:
	std::ofstream file;
};

class InputFileContainer {
public:
	InputFileContainer(const char* name, std::ios::_Openmode mode = std::ios::in) {
		file.open(name, std::ios::in | mode);
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

class OBJHelper {
	typedef std::array<uint32_t, 3> Face;
	typedef std::array<float, 3> Position;
	typedef std::array<float, 3> Normal;
	typedef std::array<float, 2> Tex;
public:
	void SetNumOfFace(uint32_t size) { faces.resize(size); }
	void SetNumOfPoint(uint32_t size) { positions.resize(size); normals.resize(size); }
	bool Finalize(const char* file) {
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
		// normal
		if (hasNormal) {
			for (const OBJHelper::Normal& nor : normals) {
				ofc << "vn " << nor[0] << ' ' << nor[1] << ' ' << nor[2] << std::endl;
			}
		}
		// face
		for (const OBJHelper::Face& face : faces) {
			ofc << "f " << face[0] + 1 << '/'; if (hasTex) ofc << face[0] + 1;  ofc << '/'; if (hasNormal) ofc << face[0] + 1;
			ofc << ' ' << face[1] + 1 << '/'; if (hasTex) ofc << face[1] + 1; ofc << '/'; if (hasNormal) ofc << face[1] + 1;
			ofc << ' ' << face[2] + 1 << '/'; if (hasTex) ofc << face[2] + 1; ofc << '/'; if (hasNormal) ofc << face[2] + 1;
			ofc << '\n';
		}
		return true;
	}
	bool SetPosition(uint32_t id, float x, float y, float z) {
		if (!state) return state;
		if (id >= positions.size()) {
			err = "pos id is out of range";
			return state = false;
		}
		positions[id][0] = x;
		positions[id][1] = y;
		positions[id][2] = z;
		return true;
	}
	bool SetFace(uint32_t id, uint32_t v1, uint32_t v2, uint32_t v3) {
		if (!state) return state;
		if (id >= faces.size()) {
			err = "face id out of range";
			return state = false;
		}
		faces[id][0] = v1;
		faces[id][1] = v2;
		faces[id][2] = v3;
		return true;
	}
	bool SetNormal(uint32_t id, float x, float y, float z) {
		if (!state) return state;
		hasNormal = true;
		if (id >= normals.size()) {
			err = "normal id is out of range";
			return state = false;
		}
		normals[id][0] = x;
		normals[id][1] = y;
		normals[id][2] = z;
		return true;
	}
	const std::string& err_msg() const { return err; }
private:
	std::string err = "";
	std::vector<Face> faces;
	std::vector<Position> positions;
	std::vector<Normal> normals;
	std::vector<Tex> texcoords;
	bool state = true;
	bool hasNormal = false;
	bool hasTex = false;
};

typedef OutputFileContainer LogFile;

template<typename T>
bool FileCompare(const char* f1, const char* f2, float threshould = 0.0001f) {
	InputFileContainer icf1(f1);
	if (!icf1.state()) return false;
	InputFileContainer icf2(f2);
	if (!icf2.state()) return false;
	T v1, v2;
	while (!icf1.isEOF() && !icf2.isEOF()) {
		icf1 >> v1;
		icf2 >> v2;
		if (abs(v1 - v2) > threshould) return false;
	}
	return true;
}

template<typename T>
inline void Test_Log(T t) {
	std::cout << t << std::endl;
}
template<typename T, typename ... Args>
inline void Test_Log(T t, Args... args) {
	std::cout << t << ' ';
	Test_Log(args...);
}

void DracoMeshToOBJ(draco::Mesh& mesh, const char* outputName);

namespace tinygltf {
	class Model;
	struct Primitive;
}
void GLTFMeshToOBJ(tinygltf::Model& m, tinygltf::Primitive& pte, const char* name);

#if defined(UTILITY_IMPLEMENTATION)
struct {
	std::unique_ptr< draco::Mesh > meshPtr;
} T_GVAR;
#endif
GE_STATE decompressMesh(const int8_t* data, size_t size);
GE_STATE analyseDecompressMesh();

#endif /* UTILITY_H */
