#ifndef ENCODER_H
#define ENCODER_H
#include <string>
#include <memory>
#include <vector>

namespace GLTF_ENCODER {
	enum GE_STATE {
		GES_OK,
		GES_ERR
	};

	struct CompressHeader {
		uint32_t cprSize;
		uint32_t glbHeaderSize = 12;
		uint32_t glbBinSize;
		uint32_t glbjsonSize;
	};

	struct EncodedMeshBufferDesc {
		uint8_t positionID = 0xffu;
		uint8_t normalID = 0xffu;
		uint8_t texcoord0ID = 0xffu;
		uint8_t tangentID = 0xffu;
		uint8_t colorID = 0xffu;
	};

	class Encoder {
	public:
		Encoder() = default;
		~Encoder() = default;
		/* call Buffer after invoking this function */
		/* or m_outputBuf data will lost after invoking this function again*/
		GE_STATE EncodeFromAsciiMemory(const std::string&);
		void SetBaseDir(const std::string& base_dir) { m_base_dir == base_dir; }
		const std::string& BaseDir() const { return m_base_dir; }
		const std::string& ErrorMsg() const { return m_err; };
		const std::string& WarnMsg() const { return m_warn; };
		/* !WARNING! */
		/* m_outbuffer won't have NO ownership of the vector(or the buffer)*/
		std::unique_ptr< std::vector<uint8_t> > Buffer() {
			return std::move(m_outputBuf);
		}
	private:
		/* encoded result */
		std::unique_ptr< std::vector<uint8_t> > m_outputBuf;
		/* last error message */
		std::string m_err;
		/* last warning message */
		std::string m_warn;
		/* base dir for loading source with relative uri */
		std::string m_base_dir = "./";
	};
}
#endif /* !ENCODER_H */
