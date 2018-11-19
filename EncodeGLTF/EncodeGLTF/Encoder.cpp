#include <draco/mesh/triangle_soup_mesh_builder.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling
#include <tinygltf/tiny_gltf.h>

#include "Encoder.h"
#include <iostream>
#include <memory>
using std::unique_ptr;
using namespace GLTF_ENCODER;

#define MCOUT(string) std::cout << string << std::endl;

/* 全局变量 */
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

/* 输入一个图元类型返回该图元涉及的属性类型 */
std::vector<AttributeDesc> GenerateAttributeDescByPrimitive(const tinygltf::Primitive&);

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) const {
	GE_STATE state = GES_OK;
	state = loadModel(jData);
	if (state != GES_OK) return state;
	state = makeMesh();
	if (state != GES_OK) return state;
	return state;
}

std::string Encoder::GetErrorMsg() const { return GVAR.err; }
std::string Encoder::GetWarnMsg() const { return GVAR.warn; }

/* private helper functions */
GE_STATE Encoder::loadModel(const std::string& jData) const {
	std::string base_dir;
	tinygltf::TinyGLTF loader;
	GE_STATE state;
	bool ret = loader.LoadASCIIFromString(&GVAR.model, &GVAR.err, &GVAR.warn, jData.c_str(), jData.size(), base_dir);
	state = ret ? GES_OK : GES_ERR;
	if (state != GES_OK) {
		MCOUT(GVAR.err);
		return state;
	}
	/* TODO: check model's data */
	/* 1. number of meshes */
	/* 2. primitive type -- what type of primitive should i support */
	size_t numOfMesh = GVAR.model.meshes.size();
	if (numOfMesh <= 0) {
		GVAR.err = "No Mesh!";
		state = GES_ERR;
	}
	
	for (const tinygltf::Mesh& meshIter : GVAR.model.meshes) {
		MCOUT("name: " << meshIter.name);
		for (const tinygltf::Primitive& primitiveIter : meshIter.primitives) {
			if (primitiveIter.mode != TINYGLTF_MODE_TRIANGLES) {
				GVAR.err = "Only support triangle currently!";
				state = GES_ERR;
			}
		}
	}

	if (state != GES_OK)
		MCOUT(GVAR.err);
	return state;
}

GE_STATE Encoder::makeMesh() const {
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

		draco::FaceIndex curFaceID;
		for (curFaceID = 0; curFaceID < numOfFaces; ++curFaceID) {
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
				MCOUT("doesn't support such type" << cmpType);
				break;
			}
		}
		else {
			/* TODO: need to support more type of attributes */
			MCOUT("doesn't support " << iter.first);
		}
		if (attDesc.attCmpCount != 0) res.push_back(attDesc);
	}
	return res;
}
