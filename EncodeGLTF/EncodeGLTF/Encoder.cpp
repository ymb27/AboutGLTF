#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>
#include <draco/compression/decode.h>
#undef DRACO_ATTRIBUTE_DEDUPLICATION_SUPPORTED

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

#ifdef TEST_COMPRESS
#include "Utility.h"
template<typename T>
inline void Test_Log(T t) {
	std::cout << t << std::endl;
}
template<typename T, typename ... Args>
inline void Test_Log(T t, Args... args) {
	std::cout << t << ' ';
	Test_Log(args...);
}

struct {
	std::unique_ptr< draco::Mesh > meshPtr;
} T_GVAR;
inline GE_STATE decompressMesh(
	const int8_t* data,
	size_t size,
	std::unique_ptr< draco::Mesh >& out); /* TEST: test if compress is complete */
inline GE_STATE analyseDecompressMesh();
inline void checkDracoMeshIndexAndPos(draco::Mesh&, const char* outputName);
inline void checkGLTFMeshIndexAndPos(tinygltf::Model&, tinygltf::Primitive&, const char* outputName);
#endif /* TEST_COMPRESS */

#ifdef TEST_COMPRESS
#define MLOG(...) Test_Log(__VA_ARGS__ )
#define CHECK_DRACO_MESH_INDEX_AND_POS(mesh, name) checkDracoMeshIndexAndPos(mesh, name)
#define CHECK_GLTF_MESH_INDEX_AND_POS(model, mesh, name) checkGLTFMeshIndexAndPos(model, mesh, name)
#else
#define MLOG(...)
#define CHECK_DRACO_MESH_INDEX_AND_POS(mesh, name)
#define CHECK_GLTF_MESH_INDEX_AND_POS(mesh, name)
#endif /* TEST_COMPRESS */


#define ARR3(name) name##[0], name##[1], name##[2]

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
inline std::vector<AttributeDesc> generateAttributeDescByPrimitive(const tinygltf::Primitive&);

inline GE_STATE loadModel(const std::string&); /* load model data from json data */
inline GE_STATE makeMesh(); /* make a Mesh object compatible with draco */
inline GE_STATE makeMesh2(); /* make a Mesh object compatible with draco ver 2.0 */
inline GE_STATE compressMesh(std::unique_ptr< std::vector<int8_t> >&); /* compress mesh maked by previous function */

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) {
	GE_STATE state = GES_OK;
	state = loadModel(jData);
	if (state != GES_OK) return state;
	//state = makeMesh();
	state = makeMesh2();
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
	/* It's invalid to compress a gltf file without any meshes */
	size_t numOfMesh = GVAR.model.meshes.size();
	if (numOfMesh <= 0) {
		GVAR.err = "No Mesh!";
		state = GES_ERR;
	}
	CHECK_GLTF_MESH_INDEX_AND_POS(GVAR.model, GVAR.model.meshes[0].primitives[0], "from gltf.obj");
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

	/* proccess for triangle primitive */
	std::vector<AttributeDesc> attDesc = generateAttributeDescByPrimitive(GVAR.model.meshes[0].primitives[0]);
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

GE_STATE makeMesh2() {
	/* TODO: need to load all the meshes in the model file! */
	/* just load the first mesh currently */
	int indexAccessorID = GVAR.model.meshes[0].primitives[0].indices;
	size_t numOfFaces = GVAR.model.accessors[indexAccessorID].count / 3;
	int bufferView = GVAR.model.accessors[indexAccessorID].bufferView;
	int buffer = GVAR.model.bufferViews[bufferView].buffer;
	int byteOffset = GVAR.model.bufferViews[bufferView].byteOffset;
	/* here assume index's type is unsigned short, which size is 2 byte */
	using INDEX_TYPE = uint16_t;
	INDEX_TYPE* indexData = reinterpret_cast<INDEX_TYPE*>((&GVAR.model.buffers[buffer].data[0]) + byteOffset);

	GVAR.mesh = std::make_unique<draco::Mesh>();
	draco::Mesh& m = (*GVAR.mesh);
	m.SetNumFaces(numOfFaces);

	size_t numOfPoints = 6070;
	m.set_num_points(numOfPoints);

	for (draco::FaceIndex fid(0); fid < numOfFaces; ++fid) {
		draco::Mesh::Face face;
		face[0] = draco::PointIndex(*(indexData + fid.value() * 3 + 0));
		face[1] = draco::PointIndex(*(indexData + fid.value() * 3 + 1));
		face[2] = draco::PointIndex(*(indexData + fid.value() * 3 + 2));
		m.SetFace(fid, face);
	}

	draco::GeometryAttribute pos;
	pos.Init(draco::GeometryAttribute::POSITION, nullptr, 3, draco::DataType::DT_FLOAT32, false, 0, 0);

	draco::GeometryAttribute nor;
	nor.Init(draco::GeometryAttribute::NORMAL, nullptr, 3, draco::DataType::DT_FLOAT32, false, 0, 0);

	draco::GeometryAttribute tex;
	tex.Init(draco::GeometryAttribute::TEX_COORD, nullptr, 2, draco::DataType::DT_FLOAT32, false,0, 0);
	
	int posID = m.AddAttribute(pos, true, numOfPoints);
	int norID = m.AddAttribute(nor, true, numOfPoints);
	int texID = m.AddAttribute(tex, true, numOfPoints);

	uint8_t* data = &GVAR.model.buffers[0].data[0];
	data = data + 64800;
	m.attribute(posID)->buffer()->Write(0, data, 72840);
	m.attribute(norID)->buffer()->Write(0, data + 137640, 72840);
	m.attribute(texID)->buffer()->Write(0, data + 210480, 48560);

	m.SetAttributeElementType(posID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
	m.SetAttributeElementType(norID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
	m.SetAttributeElementType(texID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);

	CHECK_DRACO_MESH_INDEX_AND_POS(m, "draco mesh.obj");

	return GES_OK;
}

GE_STATE compressMesh(std::unique_ptr< std::vector<int8_t> >& ptr) {
	draco::Encoder edr;
	edr.SetTrackEncodedProperties(true);
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
	MLOG("compressed size ", bufSize);
	MLOG("compressed points ", edr.num_encoded_points());
	MLOG("compressed faces ", edr.num_encoded_faces());
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
	checkDracoMeshIndexAndPos(*T_GVAR.meshPtr, "uncompressed.obj");

	/* TODO: consider there will be many meshes */
	/* you need to build model mannually */

	return GES_OK;
}
inline void checkDracoMeshIndexAndPos(draco::Mesh& mesh, const char* outputName) {
	const draco::PointAttribute* pos = mesh.attribute(draco::GeometryAttribute::POSITION);
	if (!pos) return;
	const size_t numOfPoints = mesh.num_points();
	const size_t numOfFaces = mesh.num_faces();

	OBJHelper op;
	op.SetNumOfFace(numOfFaces);
	op.SetNumOfPoint(numOfPoints);
	for (draco::FaceIndex i(0); i < numOfFaces; ++i) {
		draco::Mesh::Face face = mesh.face(i);
		op.SetFace(i.value(), face[0].value(), face[1].value(), face[2].value());
	}
	for (draco::PointIndex i(0); i < numOfPoints; ++i) {
		float vec3[3];
		pos->GetMappedValue(i, vec3);
		op.SetPosition(i.value(), vec3[0], vec3[1], vec3[2]);
	}
	if (!op.Finalize(outputName))
		MLOG(op.err_msg());
}

inline void checkGLTFMeshIndexAndPos(tinygltf::Model& m, tinygltf::Primitive& pte, const char* name) {
	if (pte.mode != TINYGLTF_MODE_TRIANGLES) return;
	int indexAccessorID = pte.indices;
	int posAccessorID = pte.attributes["POSITION"];
	tinygltf::Accessor& idxAcc = m.accessors[indexAccessorID];
	tinygltf::Accessor& posAcc = m.accessors[posAccessorID];
	size_t numOfIndex = idxAcc.count;
	size_t numOfFaces = idxAcc.count / 3;
	size_t numOfPoints = posAcc.count;

	int bufView, bufOffset;
	int bufID, byteOffset, byteStride;
	uint8_t* data = nullptr;

	OBJHelper oh;
	oh.SetNumOfFace(numOfFaces);
	oh.SetNumOfPoint(numOfPoints);
	// index
	bufView = idxAcc.bufferView;
	bufOffset = idxAcc.byteOffset;
	bufID = m.bufferViews[bufView].buffer;
	byteOffset = m.bufferViews[bufView].byteOffset;
	byteStride = m.bufferViews[bufView].byteStride;

	data = &m.buffers[bufID].data[0];
	data += byteOffset + bufOffset;
	size_t idxSize = 0;
	switch (idxAcc.componentType)
	{
	case TINYGLTF_COMPONENT_TYPE_BYTE:
		idxSize = 1;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
		idxSize = 2;
		break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		idxSize = 4;
		break;
	}

	for (int i = 0; i < numOfFaces; ++i) {
		uint32_t idx[3] = { 0 };
		memcpy_s(&idx[0], idxSize, data, idxSize); data += idxSize;
		memcpy_s(&idx[1], idxSize, data, idxSize); data += idxSize;
		memcpy_s(&idx[2], idxSize, data, idxSize); data += idxSize;
		oh.SetFace(i, ARR3(idx));
		data += byteStride;
	}

	// positions
	bufView = posAcc.bufferView;
	bufOffset = posAcc.byteOffset;
	bufID = m.bufferViews[bufView].buffer;
	byteOffset = m.bufferViews[bufView].byteOffset;
	byteStride = m.bufferViews[bufView].byteStride;

	data = &m.buffers[bufID].data[0];
	data += byteOffset + bufOffset;
	for (int i = 0; i < numOfPoints; ++i) {
		float pos[3];
		memcpy_s(pos, 12, data, 12);
		oh.SetPosition(i, ARR3(pos));
		data += 12 + byteStride;
	}
	oh.Finalize(name);
}
#endif /* TEST_COMPRESS */

/* local helper functions */
std::vector<AttributeDesc> generateAttributeDescByPrimitive(const tinygltf::Primitive& pri) {
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
			attDesc.attDataSize = attDesc.attCmpCount * 4;

		}
		else if (iter.first.compare("NORMAL") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::NORMAL;
			attDesc.attCmpCount = 3;
			attDesc.attDataType = draco::DataType::DT_FLOAT32;
			attDesc.attDataSize = attDesc.attCmpCount * 4;

		}
		else if (iter.first.compare("TEXCOORD_0") == 0 ||
				iter.first.compare("TEXCOORD_1") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::TEX_COORD;
			attDesc.attCmpCount = 2;
			int cmpType = GVAR.model.accessors[accessorID].componentType;
			switch (cmpType)
			{
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				attDesc.attDataType = draco::DataType::DT_FLOAT32;
				attDesc.attDataSize = attDesc.attCmpCount * 4;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				attDesc.attDataType = draco::DataType::DT_UINT8;
				attDesc.attDataSize = attDesc.attCmpCount * 1;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				attDesc.attDataType = draco::DataType::DT_INT16;
				attDesc.attDataSize = attDesc.attCmpCount * 2;
				break;
			default:
				/* TODO: warnning! may need some method to handle */
				MLOG("doesn't support such type" , cmpType);
				break;
			}
		}
		else if (iter.first.compare("COLOR_0") == 0) {
			attDesc.attGeoType = draco::GeometryAttribute::COLOR;
			int cmpCount = GVAR.model.accessors[accessorID].type;
			int cmpType = GVAR.model.accessors[accessorID].componentType;
			if (cmpCount == TINYGLTF_TYPE_VEC3) {
				attDesc.attCmpCount = 3;
				switch (cmpType) {
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					attDesc.attDataType = draco::DataType::DT_FLOAT32;
					attDesc.attDataSize = attDesc.attCmpCount * 4;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					attDesc.attDataType = draco::DataType::DT_UINT8;
					attDesc.attDataSize = attDesc.attCmpCount * 1;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					attDesc.attDataType = draco::DataType::DT_UINT16;
					attDesc.attDataSize = attDesc.attCmpCount * 2;
					break;
				default:
					/* TODO: need to handle invalid component type */
					MLOG("invalid component type ", iter.first);
					break;
				}
			}
			else if (cmpCount == TINYGLTF_TYPE_VEC4) {
				attDesc.attCmpCount = 4;
				switch (cmpType)
				{
				case TINYGLTF_COMPONENT_TYPE_FLOAT:
					attDesc.attDataType = draco::DataType::DT_FLOAT32;
					attDesc.attDataSize = attDesc.attCmpCount * 4;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
					attDesc.attDataType = draco::DataType::DT_UINT8;
					attDesc.attDataSize = attDesc.attCmpCount * 1;
					break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
					attDesc.attDataType = draco::DataType::DT_UINT16;
					attDesc.attDataSize = attDesc.attCmpCount * 2;
					break;
				default:
					/* TODO: need to handle invalid component type */
					MLOG("invalid component type ", iter.first);
					break;
				}
			}
			else {
				/* TODO: need to handle invalid component count */
				MLOG("invalid component count ", iter.first);
			}
		}
		/* TODO: Currently don't consider animation */
		/* So ignore JOINTS_0 and WEIGHTS_0 */
		else {
			/* TODO: invalid primitive.attriute type */
			MLOG("doesn't support ", iter.first);
		}
		if (attDesc.attCmpCount != 0) res.push_back(attDesc);
	}
	return res;
}
