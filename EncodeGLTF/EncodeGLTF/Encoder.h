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

	struct MeshBufferHeader {
		const uint8_t headerSize = sizeof(MeshBufferHeader);
		uint32_t bufferLength = 0; /* length of the buffer: header + data */
		uint8_t positionID = 0xffu;
		uint8_t normalID = 0xffu;
		uint8_t texcoordID = 0xffu;
		uint8_t tangentID = 0xffu;
		uint8_t colorID = 0xffu;
	};

	class Encoder {
	public:
		Encoder() = default;
		~Encoder() = default;

		GE_STATE EncodeFromAsciiMemory(const std::string&);
		const std::string& ErrorMsg() const;
		const std::string& WarnMsg() const;
		/* !WARNING! */
		/* m_outbuffer won't have NO ownership of the vector(or the buffer)*/
		std::unique_ptr< std::vector<int8_t> > Buffer();
	private:
		/* merge all buffers in m_buffers into m_outBuffer and add a header */
		GE_STATE finalize();
		/* add a buffer into m_buffers and add a sub header by the way */
		GE_STATE addBuffer(std::unique_ptr<std::vector<int8_t> > inputBuffer);
		/* encoded result */
		std::unique_ptr< std::vector<int8_t> > m_outBuffer;
		/* store each compressed buffer */
		std::vector< std::unique_ptr<std::vector<int8_t> > > m_buffers;
		/* last error message */
		std::string m_err;
		/* last warning message */
		std::string m_warn;
	};
}
#endif /* !ENCODER_H */
