#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
using std::cout;
using std::endl;
namespace GLB_PROC {
	// GLB 文件头大小
	const size_t HEADER_SIZE = 12;
	const size_t MAX_FILE_SIZE = UINT32_MAX;
	const uint32_t HEADER_MAGIC = 0x46546C67ui32;
	const uint32_t TYPE_JSON = 0x4E4F534Aui32;
	const uint32_t TYPE_BINARY = 0x004E4942ui32;
	struct GLB_HEADER {
		uint32_t magic;
		uint32_t version;
		uint32_t length;
	};
	struct CHUNK {
		uint32_t length;
		uint32_t type;
		uint32_t* data;
	};
	class FileContainer {
	public:
		FileContainer(std::string const& filePath)
			:File(filePath, std::ios::binary) {}
		~FileContainer() {
			if (File.is_open()) File.close();
		}
		size_t GetFileSize() {
			if (!File.is_open()) return 0;
			File.seekg(0, File.end);
			const std::streampos fileSize = File.tellg();
			File.seekg(0, File.beg);
			return static_cast<size_t>(fileSize);
		}
		std::ifstream File;
	};

	class Encoder {
	public:
		bool LoadBinary(std::string const& filePath) {
			FileContainer fc(filePath);
			if (!fc.File.is_open()) {
				cout << "no such file" << endl;
				return false;
			}
			const size_t fileSize = fc.GetFileSize();
			if (fileSize < HEADER_SIZE) {
				cout << "invalid glb file" << endl;
				return false;
			}
			if (fileSize > MAX_FILE_SIZE) {
				cout << "file is too big" << endl;
				return false;
			}
			m_binary.resize(fileSize);
			fc.File.read(reinterpret_cast<char*>(&m_binary[0]), fileSize);
			analyseHeader();
			analyseJsonChunk();
			return true;
		}
	private:
		inline bool analyseHeader() {
			memcpy_s(&m_header, sizeof(m_header),
				&m_binary[0], sizeof(m_header));
			if (m_header.magic != HEADER_MAGIC) {
				cout << "invalid glb header!" << endl;
				return false;
			}
			return true;
		}
		inline bool analyseJsonChunk() {
			memcpy_s(&json_chunk, sizeof(json_chunk),
				&m_binary[sizeof(m_header)], sizeof(json_chunk));
			json_chunk.data = reinterpret_cast<uint32_t*>(&m_binary[5 * sizeof(uint32_t)]);
			return true;
		}
	private:
		std::vector<uint8_t> m_binary;
		GLB_HEADER m_header;
		CHUNK json_chunk;
	};
}