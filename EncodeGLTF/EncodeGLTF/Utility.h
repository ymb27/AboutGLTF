#pragma once
#include <array>
#include <vector>
#include <string>
#include <fstream>
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
	InputFileContainer(const char* name) {
		file.open(name, std::ios::in);
	}
	~InputFileContainer() {
		if (file.is_open()) file.close();
	}
	const bool state() const { return file.is_open(); }
	const bool isEOF() const { return  file.eof(); }
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
	void SetNumOfPoint(uint32_t size) { positions.resize(size); }
	bool Finalize(const char* fileName);
	bool SetPosition(uint32_t id, float x, float y, float z) {
		if (!state) return state;
		if (id >= positions.size()) {
			err = "pos id out of range";
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
	const std::string& err_msg() const { return err; }
private:
	bool state = true;
	std::string err = "";
	std::vector<Face> faces;
	std::vector<Position> positions;
	std::vector<Normal> normals;
	std::vector<Tex> texcoords;
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