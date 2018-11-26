#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>
#include <draco/compression/decode.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling
#include "tinyGLTF_wrapper.h"

#include "Encoder.h"
#include <iostream>
#include <utility>
#include <memory>
using std::unique_ptr;
using namespace GLTF_ENCODER;

#define TEST_COMPRESS

#ifdef TEST_COMPRESS
#include "Utility.h"
#define MLOG(...) Test_Log(__VA_ARGS__ )
#define DRACO_MESH_TO_OBJ(mesh, name) DracoMeshToOBJ(mesh, name)
#define GLTF_MESH_TO_OBJ(model, mesh, name) GLTFMeshToOBJ(model, mesh, name)
#define DECOMPRESS_TEST(data, size) decompressMesh(data, size)
#define ANALYSE_DECOMPRESS() analyseDecompressMesh()
#else
#define MLOG(...)
#define DRACO_MESH_TO_OBJ(mesh, name)
#define GLTF_MESH_TO_OBJ(model, mesh, name)
#define DECOMPRESS_TEST(data, size) GES_OK
#define ANALYSE_DECOMPRESS() GES_OK
#endif /* TEST_COMPRESS */


#define ARR3(name) name##[0], name##[1], name##[2]

/* global variable */
struct {
	tinygltf::Model model;
	std::string err;
	std::string warn;
	unique_ptr<draco::Mesh> mesh;
} GVAR;

/* load model data from json data */
inline GE_STATE loadModel(const std::string&);
/* Traverse all the primitives of meshes in gltf file */
inline GE_STATE traverseModel();
/* make a draco mesh according to the triangle primitive */
/* caller should guarantee that primitive is triangle */
inline GE_STATE makeMesh(tinygltf::Primitive& triangleMeshPrimitive);
/* compress mesh maked by previous function */
inline GE_STATE compress(std::unique_ptr< std::vector<int8_t> >&);

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) {
	GE_STATE state = GES_OK;
	state = loadModel(jData);
	if (state != GES_OK) return state;
	state = makeMesh(GVAR.model.meshes[0].primitives[0]);
	if (state != GES_OK) return state;
	state = compress(m_outBuffer);
	if (state != GES_OK) return state;

	state = DECOMPRESS_TEST(&(*m_outBuffer)[0], (*m_outBuffer).size());
	if (state != GES_OK) return state;
	state = ANALYSE_DECOMPRESS();
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
	GLTF_MESH_TO_OBJ(GVAR.model, GVAR.model.meshes[0].primitives[0], "from gltf.obj");
	return state;
}

GE_STATE traverseModel() {
	for (tinygltf::Mesh& mesh : GVAR.model.meshes) {
		for (tinygltf::Primitive& pri : mesh.primitives) {
			if (pri.mode == TINYGLTF_MODE_TRIANGLES) {

			}
		}
	}
}

GE_STATE makeMesh(tinygltf::Primitive& pri) {

	size_t numOfFaces = tinygltf_wrapper::NumOfFaces(pri, GVAR.model);
	size_t numOfPoints = tinygltf_wrapper::NumOfPoints(pri, GVAR.model);

	GVAR.mesh = std::make_unique<draco::Mesh>();
	draco::Mesh& m = (*GVAR.mesh);
	m.SetNumFaces(numOfFaces);
	m.set_num_points(numOfPoints);

	/* processing index */
	/* doesn't support sparse accessor */
	tinygltf_wrapper::IndexContainer ic = tinygltf_wrapper::GetIndices(pri, GVAR.model);
	for (draco::FaceIndex fid(0); fid < numOfFaces; ++fid) {
		draco::Mesh::Face face;
		face[0] = draco::PointIndex(ic[fid.value() * 3 + 0]);
		face[1] = draco::PointIndex(ic[fid.value() * 3 + 1]);
		face[2] = draco::PointIndex(ic[fid.value() * 3 + 2]);
		m.SetFace(fid, face);
	}
	/* processing attributes */
	for (const auto& iter : pri.attributes) {
		draco::GeometryAttribute::Type geoAttType;
		draco::DataType geoDataType;
		uint32_t specAttType;
		tinygltf::Accessor& attAcc = GVAR.model.accessors[iter.second];
		int8_t numOfCmp = 0;
		if (!iter.first.compare("POSITION")) {
			geoAttType = draco::GeometryAttribute::POSITION;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 3;
		}
		else if (!iter.first.compare("NORMAL")) {
			geoAttType = draco::GeometryAttribute::NORMAL;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 3;
		}
		else if (!iter.first.compare("TEXCOORD_0")) {
			geoAttType = draco::GeometryAttribute::TEX_COORD;
			switch (attAcc.componentType) {
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				geoDataType = draco::DataType::DT_FLOAT32;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				geoDataType = draco::DataType::DT_UINT8;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				geoDataType = draco::DataType::DT_UINT16;
				break;
			}
			numOfCmp = 2;
		}
		else if (!iter.first.compare("TANGENT")) {
			geoAttType = draco::GeometryAttribute::INVALID;
			specAttType = GLTF_EXTENT_ATTRIBUTE::TANGENT;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 4;
		}
		else {
			/* TODO: need to support other attribute */
			MLOG("currently doesn't support", iter.first);
			continue;
		}

		if (geoAttType != draco::GeometryAttribute::INVALID) {
			draco::GeometryAttribute geoAtt;
			/* stride and offset will be set according to data type */
			/* and components when add attribute to mesh */
			geoAtt.Init(geoAttType, nullptr, numOfCmp, geoDataType, attAcc.normalized, 0, 0);
			int attID = m.AddAttribute(geoAtt, true, numOfPoints);
			tinygltf_wrapper::DataContainer dc = tinygltf_wrapper::DataContainer::Create(attAcc, GVAR.model);
			m.attribute(attID)->buffer()->Write(0, &dc.packBuffer()[0], dc.size() * dc.count());
			m.SetAttributeElementType(attID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
		}
		else {
			std::unique_ptr<draco::PointAttribute> pntAtt = std::make_unique<draco::PointAttribute>();
			pntAtt->Init(draco::GeometryAttribute::GENERIC, nullptr, numOfCmp, geoDataType, attAcc.normalized, 0, 0);
			/* WARNNING: after set attribute pntAtt won't have any ownership of PointAttribute */
			m.SetAttribute(specAttType, std::move(pntAtt));
			tinygltf_wrapper::DataContainer dc = tinygltf_wrapper::DataContainer::Create(attAcc, GVAR.model);
			m.attribute(specAttType)->buffer()->Write(0, &dc.packBuffer()[0], dc.size() * dc.count());
			m.SetAttributeElementType(specAttType, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
		}
	}

	DRACO_MESH_TO_OBJ(m, "draco mesh.obj");

	return GES_OK;
}

GE_STATE compress(std::unique_ptr< std::vector<int8_t> >& ptr) {
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

