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
	/* attributes are not defined by draco */
	enum GLTF_EXTENT_ATTRIBUTE {
		TANGENT = 200
	};

	struct EncoderHeader {
		uint8_t headerSize;
		uint8_t numOfbuffer;
		uint8_t headerOffset;
	};

	struct BufferHeader {
		uint8_t headerSize;
		uint8_t headerOffset;
		uint32_t bufferLength;
	};

	class Encoder {
	public:
		GE_STATE EncodeFromAsciiMemory(const std::string&);
		std::string GetErrorMsg() const;
		std::string GetWarnMsg() const;
		/* !WARNING! */
		/* m_outbuffer won't have NO ownership of the vector(or the buffer)*/
		std::unique_ptr< std::vector<int8_t> > Buffer();
	private:
		/* encoded result */
		std::unique_ptr< std::vector<int8_t> > m_outBuffer;
	};
}
#endif /* !ENCODER_H */
