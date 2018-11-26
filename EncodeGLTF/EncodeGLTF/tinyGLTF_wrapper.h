﻿#ifndef TINYGLTF_WRAPPER
#define TINYGLTF_WRAPPER
#include <tinygltf/tiny_gltf.h>

namespace tinygltf_wrapper {

	const uint32_t INVALID_UINT_32 = 0xffffffffu;
	const uint8_t INVALID_UINT_8 = 0xffu;

	/* make sense to triangle primitive */
	inline uint32_t NumOfFaces(const tinygltf::Primitive& p,
		const tinygltf::Model& m) {
		return m.accessors[p.indices].count / 3;
	}

	/* caller should guarantee that primitive has position attribute */
	inline uint32_t NumOfPoints(const tinygltf::Primitive& p,
		const tinygltf::Model& m) {
		int posAccId = p.attributes.find("POSITION")->second;
		return m.accessors[posAccId].count;
	}

	/* caller should  guarantee that primitive has indices attribute */
	inline tinygltf::Accessor& IndexAccessor(const tinygltf::Primitive& p,
		tinygltf::Model& m) {
		return m.accessors[p.indices];
	}

	/* return the size of different component type in bytes */
	inline uint8_t ComponentSize(int type) {
		switch (type) {
		case TINYGLTF_COMPONENT_TYPE_BYTE:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return 1;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return 2;
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			return 4;
		default:
			assert(false);
		}
		return INVALID_UINT_8;
	}

	inline uint8_t ComponentNum(int type) {
		switch (type) {
		case TINYGLTF_TYPE_SCALAR:
			return 1;
		case TINYGLTF_TYPE_VEC2:
			return 2;
		case TINYGLTF_TYPE_VEC3:
			return 3;
		case TINYGLTF_TYPE_VEC4:
		case TINYGLTF_TYPE_MAT2:
			return 4;
		case TINYGLTF_TYPE_MAT3:
			return 9;
		case TINYGLTF_TYPE_MAT4:
			return 16;
		}
		return INVALID_UINT_8;
	}

	class DataContainer {
	public:
		static DataContainer Create(tinygltf::Accessor& acc, tinygltf::Model& m) {
			int byteOffset = acc.byteOffset;

			tinygltf::BufferView& bufView = m.bufferViews[acc.bufferView];
			tinygltf::Buffer& buf = m.buffers[bufView.buffer];
			
			uint8_t size = ComponentSize(acc.componentType) * ComponentNum(acc.type);
			return DataContainer(&buf.data[0] + bufView.byteOffset + acc.byteOffset,
				size, static_cast<uint8_t>(bufView.byteStride), acc.count);
		}
	public:
		DataContainer(uint8_t* data, uint8_t size, uint8_t stride, uint32_t count)
			: m_data(data), m_stride(stride ? stride : size), m_count(count), m_size(size) {}
		inline uint32_t count() const { return m_count; }
		inline uint8_t* data() const { return m_data; }
		inline uint8_t stride() const { return m_stride; }
		inline uint8_t size() const { return m_size; }
		std::vector<uint8_t> packBuffer() const {
			std::vector<uint8_t> res;
			res.resize(m_count * m_size);
			uint8_t* res_ptr = &res[0];
			for (uint32_t i = 0; i < m_count; ++i) {
				memcpy_s(res_ptr, m_size, m_data + i * m_stride, m_size);
				res_ptr += m_size;
			}
			return res;
		}
	private:
		uint32_t m_count = 0;
		uint8_t * m_data = nullptr;
		uint8_t m_stride = 0;
		uint8_t m_size = 0;
	};

	/* helper class for retrieve index from primitive's index buffer */
	class IndexContainer{
	public:
		IndexContainer(DataContainer dc) : dc(dc) {}
		IndexContainer(uint8_t* data, uint8_t size, uint8_t stride, uint32_t count)
			: dc(data, size, stride, count) {}
		uint32_t operator[](uint32_t id) {
			assert(id < dc.count());
			uint8_t* d = dc.data() + id * dc.stride();
			switch (dc.size())
			{
			case 1:
				return static_cast<uint32_t>(*d);
			case 2:
				return static_cast<uint32_t>(*reinterpret_cast<uint16_t*>(d));
			case 4:
				return static_cast<uint32_t>(*reinterpret_cast<uint32_t*>(d));
			default:
				assert(false);
				break;
			}
			return INVALID_UINT_32;
		}
	private:
		DataContainer dc;
	};

	/* caller should guarantee that primitive has indices attribute */
	inline IndexContainer GetIndices(const tinygltf::Primitive& p,
		tinygltf::Model& m) {
		tinygltf::Accessor& acc = IndexAccessor(p, m);
		return IndexContainer(DataContainer::Create(acc, m));
	}
}

#endif /*TINYGLTF_WRAPPER*/
