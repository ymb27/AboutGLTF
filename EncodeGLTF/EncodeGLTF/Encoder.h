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

	struct EncoderHeader {
		const uint8_t headerSize = sizeof(EncoderHeader);
		uint32_t numOfbuffer = 0; /* so total number of primitive should not more than 2 ^ 32 */
		uint32_t bufferLength = 0; /* length of all data: header + buffers */
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
	};
}
#endif /* !ENCODER_H */
