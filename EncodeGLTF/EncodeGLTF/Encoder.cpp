#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>
#include <draco/compression/decode.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling
#include <tinygltf/tiny_gltf.h>

#include "Encoder.h"
#include <iostream>
#include <utility>
#include <memory>
using std::unique_ptr;
using namespace GLTF_ENCODER;

#define TEST_COMPRESS
#define MLOG(string) std::cout << string << std::endl

/* global variable */
struct {
	tinygltf::Model model;
	std::string err;
	std::string warn;
	unique_ptr<draco::Mesh> mesh;
} GVAR;

/* 属性描述 */
/* 利用tinytf读取的结果，构造draco能接收的属性描述信息 */
/* 该结构依赖于Draco */
struct AttributeDesc {
	draco::GeometryAttribute::Type attGeoType;
	draco::DataType attDataType;
	uint8_t* data; /* pointer to the value buffer */
	uint8_t attDataSize; /* bytes of data */
	int8_t attCmpCount;
};

/* helper functions */
/* 输入一个图元类型返回该图元涉及的属性类型 */
std::vector<AttributeDesc> GenerateAttributeDescByPrimitive(const tinygltf::Primitive&);

inline GE_STATE loadModel(const std::string&); /* load model data from json data */
inline GE_STATE makeMesh(); /* make a Mesh object compatible with draco */
inline GE_STATE compressMesh(std::unique_ptr< std::vector<int8_t> >&); /* compress mesh maked by previous function */

#ifdef TEST_COMPRESS
struct {
	std::unique_ptr< draco::Mesh > meshPtr;
} T_GVAR;
inline GE_STATE decompressMesh(
	const int8_t* data,
	size_t size,
	std::unique_ptr< draco::Mesh >& out); /* TEST: test if compress is complete */
inline GE_STATE analyseDecompressMesh();
#endif /* TEST_COMPRESS */

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) {
	GE_STATE state = GES_OK;
	state = loadModel(jData);
	if (state != GES_OK) return state;
	state = makeMesh();
	if (state != GES_OK) return state;
	state = compressMesh(m_outBuffer);
	if (state != GES_OK) return state;

#ifdef TEST_COMPRESS
	state = decompressMesh(&(*m_outBuffer)[0], (*m_outBuffer).size(), T_GVAR.meshPtr);
	if (state != GES_OK) return state;
	state = analyseDecompressMesh();
#endif /* TEST_COMPRESS */
	return state;
}

std::string Encoder::GetErrorMsg() const { return GVAR.err; }
std::string Encoder::GetWarnMsg() const { return GVAR.warn; }

/* private helper functions */
GE_STATE loadModel(const std::string& jData) {
	std::string base_dir;
	tinygltf::TinyGLTF loader;
	GE_STATE state;
	bool ret = loader.LoadASCIIFromString(&GVAR.model, &GVAR.err, &GVAR.warn, jData.c_str(), jData.size(), base_dir);
	state = ret ? GES_OK : GES_ERR;
	if (state != GES_OK)	return state;
	/* TODO: check model's data */
	/* 1. number of meshes */
	/* 2. primitive type -- what type of primitive should i support */
	size_t numOfMesh = GVAR.model.meshes.size();
	if (numOfMesh <= 0) {
		GVAR.err = "No Mesh!";
		state = GES_ERR;
	}
	
	for (const tinygltf::Mesh& meshIter : GVAR.model.meshes) {
		MLOG("name: " << meshIter.name);
		for (const tinygltf::Primitive& primitiveIter : meshIter.primitives) {
			if (primitiveIter.mode != TINYGLTF_MODE_TRIANGLES) {
				GVAR.err = "Only support triangle currently!";
				state = GES_ERR;
			}
		}
	}
	return state;
}

GE_STATE makeMesh()  {
	/* TODO: need to load all the meshes in the model file! */
	/* just load the first mesh currently */
	int indexAccessorID  = GVAR.model.meshes[0].primitives[0].indices;
	size_t numOfFaces = GVAR.model.accessors[indexAccessorID].count / 3;
	int bufferView = GVAR.model.accessors[indexAccessorID].bufferView;
	int buffer = GVAR.model.bufferViews[bufferView].buffer;
	int byteOffset = GVAR.model.bufferViews[bufferView].byteOffset;
	/* here assume index's type is unsigned short, which size is 2 byte */
	using INDEX_TYPE = uint16_t;
	INDEX_TYPE* indexData = reinterpret_cast<INDEX_TYPE*>((&GVAR.model.buffers[buffer].data[0]) + byteOffset);
	draco::TriangleSoupMeshBuilder meshBuilder;
	meshBuilder.Start(numOfFaces);

	/* TODO: I don't know if there are two triangle primitives */
	std::vector<AttributeDesc> attDesc = GenerateAttributeDescByPrimitive(GVAR.model.meshes[0].primitives[0]);
	for (AttributeDesc& iter : attDesc) {
		int attID = meshBuilder.AddAttribute(
			iter.attGeoType, iter.attCmpCount,
			iter.attDataType);

		for (draco::FaceIndex curFaceID(0); curFaceID < numOfFaces; ++curFaceID) {
			INDEX_TYPE c1 = *(indexData + curFaceID.value() * 3 + 0);
			INDEX_TYPE c2 = *(indexData + curFaceID.value() * 3 + 1);
			INDEX_TYPE c3 = *(indexData + curFaceID.value() * 3 + 2);
			meshBuilder.SetAttributeValuesForFace(attID, curFaceID,
				iter.data + c1 * iter.attDataSize,
				iter.data + c2 * iter.attDataSize,
				iter.data + c3 * iter.attDataSize);
		}
	}
	GVAR.mesh = meshBuilder.Finalize();
	return GES_OK;
}

GE_STATE compressMesh(std::unique_ptr< std::vector<int8_t> >& ptr) {
	draco::Encoder edr;
	draco::EncoderBuffer eBuf;
	draco::Status status;
	status = edr.EncodeMeshToBuffer(*GVAR.mesh, &eBuf);
	if (!status.ok()) {
		GVAR.err = status.error_msg_string();
		return GES_ERR;
	}
	ptr = std::make_unique< std::vector<int8_t> >();
	size_t bufSize = eBuf.size();
	(*ptr).resize(bufSize);
	memcpy_s(&(*ptr)[0], bufSize, eBuf.data(), bufSize);
	MLOG(bufSize);
	return GES_OK;
}

#ifdef TEST_COMPRESS
inline GE_STATE decompressMesh(
	const int8_t* data,
	size_t size,
	std::unique_ptr< draco::Mesh >& out ) {
	draco::Decoder ddr;
	draco::DecoderBuffer deBuf;
	deBuf.Init(reinterpret_cast<const char*>(data), size);
	draco::StatusOr< std::unique_ptr<draco::Mesh> >&& rStatus
		= ddr.DecodeMeshFromBuffer(&deBuf);
	if (!rStatus.ok()) {
		GVAR.err = rStatus.status().error_msg_string();
		return GES_ERR;
	}
	out = std::move(rStatus).value();
	return GES_OK;
}

/* TODO validate the compress result by decompress it and recreate a gltf file */
inline GE_STATE analyseDecompressMesh() {
	draco::Mesh& mesh = (*T_GVAR.meshPtr);
	const draco::PointAttribute* ptr_position = mesh.GetNamedAttribute(draco::GeometryAttribute::POSITION);
	const draco::PointAttribute* ptr_normal = mesh.GetNamedAttribute(draco::GeometryAttribute::NORMAL);
	const draco::PointAttribute* ptr_texcoord0 = mesh.GetNamedAttribute(draco::GeometryAttribute::TEX_COORD);
	const size_t numOfPoints = mesh.num_points();
	const size_t numOfFaces = mesh.num_faces();

	draco::PointIndex pid(6069);
	draco::AttributeValueIndex avi = ptr_position->mapped_index(pid);


	tinygltf::Model outModel;
	tinygltf::Buffer buf;
	tinygltf::TinyGLTF writer;

	/* TODO: consider there will be many meshes */
	/* you need to build model mannually */

	return GES_OK;
}
#endif /* TEST_COMPRESS */

/* local helper functions */
std::vector<AttributeDesc> GenerateAttributeDescByPrimitive(const tinygltf::Primitive& pri) {
	std::vector<AttributeDesc> res;
	int accessorID = 0;
	int bufferView = 0, buffer = 0, byteOffset = 0;
	for (const auto& iter : pri.attributes) {
		AttributeDesc attDesc;
		attDesc.attCmpCount = 0; /* mark that this varriable was not initialized */
		/* just support position, normal and the first set texture coordinate currently */
		accessorID = iter.second;
		bufferView = GVAR.model.accessors[accessorID].bufferView;
		buffer = GVAR.model.bufferViews[bufferView].buffer;
		byteOffset = GVAR.model.bufferViews[bufferView].byteOffset;

		attDesc.data = (&GVAR.model.buffers[buffer].data[0]) + byteOffset;
		if (iter.first.compare("POSITION") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::POSITION;
			attDesc.attCmpCount = 3;
			/* since gltf support float32array */
			attDesc.attDataType = draco::DataType::DT_FLOAT32;
			attDesc.attDataSize = 3 * 4;

		}
		else if (iter.first.compare("NORMAL") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::NORMAL;
			attDesc.attCmpCount = 3;
			attDesc.attDataType = draco::DataType::DT_FLOAT32;
			attDesc.attDataSize = 3 * 4;

		}
		else if (iter.first.compare("TEXCOORD_0") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::TEX_COORD;
			attDesc.attCmpCount = 2;
			int cmpType = GVAR.model.accessors[accessorID].componentType;
			switch (cmpType)
			{
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				attDesc.attDataType = draco::DataType::DT_FLOAT32;
				attDesc.attDataSize = 2 * 4;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				attDesc.attDataType = draco::DataType::DT_UINT8;
				attDesc.attDataSize = 2 * 1;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				attDesc.attDataType = draco::DataType::DT_INT8;
				attDesc.attDataSize = 2 * 1;
				break;
			default:
				/* TODO: warnning! may need some method to handle */
				MLOG("doesn't support such type" << cmpType);
				break;
			}
		}
		else {
			/* TODO: need to support more type of attributes */
			MLOG("doesn't support " << iter.first);
		}
		if (attDesc.attCmpCount != 0) res.push_back(attDesc);
	}
	return res;
}
