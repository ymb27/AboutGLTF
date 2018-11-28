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
/* LOG for developing data */
#define MLOG(...) Test_Log(__VA_ARGS__ )
#define DRACO_MESH_TO_OBJ(mesh, name) DracoMeshToOBJ(mesh, name)
#define GLTF_MESH_TO_OBJ(model, mesh, name) GLTFMeshToOBJ(model, mesh, name)
#define DECOMPRESS(data) decompress(data)
#define DECOMPRESS_MESH(data, size) decompressMesh(data, size)
#define ANALYSE_DECOMPRESS() analyseDecompressMesh()
#else
#define MLOG(...)
#define DRACO_MESH_TO_OBJ(mesh, name)
#define GLTF_MESH_TO_OBJ(model, mesh, name)
#define DECOMPRESS(data)
#define DECOMPRESS_MESH(data, size) GES_OK
#define ANALYSE_DECOMPRESS() GES_OK
#endif /* TEST_COMPRESS */


#define ARR3(name) name##[0], name##[1], name##[2]

/* global variable */
struct {
	std::unique_ptr< tinygltf::Model > model;
	std::string* err;
	std::string* warn;
} GVAR;

/* load model data from json data */
inline GE_STATE loadModel(const std::string&);
/* make a draco mesh according to the triangle primitive */
/* caller should guarantee that primitive is triangle */
inline GE_STATE makeMesh(const tinygltf::Primitive& triangleMeshPrimitive, 
	std::unique_ptr<draco::Mesh>& outputMesh, EncodedMeshBufferDesc& header);
/* compress mesh maked by previous function */
inline GE_STATE compressMesh(std::unique_ptr<draco::Mesh>& inputMesh,
	std::unique_ptr< std::vector<uint8_t> >& outputBuffer);
/* targetPri is the origin primitive, this function will add extension to it */
/* and reset its attributes' bufferView ID since it won't use uncompress data any more */
inline GE_STATE setDracoCompressPrimitive(tinygltf::Primitive& targetPri, tinygltf::Model& targetMod,
	const EncodedMeshBufferDesc& desc, 
	std::unique_ptr<std::vector<uint8_t> >& compressData,
	std::unique_ptr<std::vector<uint32_t> >& compressIndices);

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) {
	GVAR.err = &m_err;
	GVAR.warn = &m_warn;
	GE_STATE state = GES_OK;
	state = loadModel(jData);
	if (state != GES_OK) return state;
	/* preallocate space for buffer-pointer container */
	/* assume that each mesh has two primitives */
	m_buffers.reserve(GVAR.model->meshes.size() * 2);
	/* Traversal model file and compressed each primitive */
	/* TODO: currently only support triangle and ignore its material */
	for (tinygltf::Mesh& mesh : GVAR.model->meshes) {
		for (tinygltf::Primitive& pri : mesh.primitives) {
			if (pri.mode == TINYGLTF_MODE_TRIANGLES) {
				EncodedMeshBufferDesc header;
				/* build a mesh according to primitive's attributes */
				std::unique_ptr<draco::Mesh> mesh;
				state = makeMesh(pri, mesh, header);
				if (state != GES_OK) return state;
				/* buffer that contain this primitives compressed data */
				/* It needs to be merge within m_outBuffer */
				std::unique_ptr<std::vector<uint8_t> > compressBuffer;
				state = compressMesh(mesh, compressBuffer);
				if (state != GES_OK) return state;
				/* set compressBuffer to gltf file*/

			}
		}
	}
	/* merge compressBuffer into m_outBuffer */
	state = finalize();
	if (state != GES_OK) return state;

	state = DECOMPRESS(m_outBuffer->data());
	if (state != GES_OK) return state;
	return state;
}

const std::string& Encoder::ErrorMsg() const { return m_err; }
const std::string& Encoder::WarnMsg() const { return m_warn; }

GE_STATE Encoder::addBuffer(std::unique_ptr< std::vector<int8_t> > inputBuffer) {
	m_buffers.push_back(std::move(inputBuffer));
	return GES_OK;
}

GE_STATE Encoder::finalize() {
	EncoderHeader eh;
	eh.numOfbuffer = static_cast<uint32_t>(m_buffers.size());
	eh.bufferLength = sizeof(EncoderHeader);
	for (const std::unique_ptr< std::vector<int8_t> >& iter : m_buffers) {
		eh.bufferLength += iter->size();
	}
	m_outBuffer = std::make_unique< std::vector<int8_t> >();
	size_t bufSize = eh.bufferLength;
	m_outBuffer->resize(bufSize);
	/* copy header data into buffer */
	int8_t* bufData = m_outBuffer->data();
	memcpy_s(bufData, sizeof(EncoderHeader),
		&eh, sizeof(EncoderHeader));
	/* merge each buffer data into outBuffer */
	bufData += sizeof(EncoderHeader);
	for (const std::unique_ptr< std::vector<int8_t> >& iter : m_buffers) {
		memcpy_s(bufData, iter->size(),
			iter->data(), iter->size());
		bufData += iter->size();
	}
	return GES_OK;
}

/* private helper functions */
GE_STATE loadModel(const std::string& jData) {
	std::string base_dir;
	tinygltf::TinyGLTF loader;
	GE_STATE state;
	bool ret = loader.LoadASCIIFromString(GVAR.model.get(), GVAR.err, GVAR.warn, jData.c_str(), jData.size(), base_dir);
	state = ret ? GES_OK : GES_ERR;
	if (state != GES_OK)	return state;
	/* It's invalid to compress a gltf file without any meshes */
	size_t numOfMesh = GVAR.model->meshes.size();
	if (numOfMesh <= 0) {
		*GVAR.err = "No Mesh!";
		state = GES_ERR;
	}
	GLTF_MESH_TO_OBJ(*GVAR.model, GVAR.model->meshes[0].primitives[0], "from gltf.obj");
	return state;
}

GE_STATE makeMesh(const tinygltf::Primitive& pri,
	std::unique_ptr<draco::Mesh>& outputMesh,
	EncodedMeshBufferDesc& header) {

	size_t numOfFaces = tinygltf_wrapper::NumOfFaces(pri, *GVAR.model);
	size_t numOfPoints = tinygltf_wrapper::NumOfPoints(pri, *GVAR.model);

	outputMesh = std::make_unique<draco::Mesh>();
	draco::Mesh& m = (*outputMesh);
	m.SetNumFaces(numOfFaces);
	m.set_num_points(numOfPoints);

	enum {
		TANGENT
	} extentPointType;

	/* processing index */
	/* doesn't support sparse accessor */
	tinygltf_wrapper::IndexContainer ic = tinygltf_wrapper::GetIndices(pri, *GVAR.model);
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
		tinygltf::Accessor& attAcc = GVAR.model->accessors[iter.second];
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
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 4;
			extentPointType = TANGENT;
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
			switch (geoAttType)
			{
			case draco::GeometryAttribute::POSITION:
				header.positionID = attID;
				break;
			case draco::GeometryAttribute::NORMAL:
				header.normalID = attID;
				break;
			case draco::GeometryAttribute::COLOR:
				header.colorID = attID;
				break;
			case draco::GeometryAttribute::TEX_COORD:
				header.texcoordID = attID;
				break;
			}
			tinygltf_wrapper::DataContainer dc = tinygltf_wrapper::DataContainer::Create(attAcc, *GVAR.model);
			m.attribute(attID)->buffer()->Write(0, &dc.packBuffer()[0], dc.size() * dc.count());
			m.SetAttributeElementType(attID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
		}
		else {
			tinygltf_wrapper::DataContainer dc = tinygltf_wrapper::DataContainer::Create(attAcc, *GVAR.model);
			std::unique_ptr<draco::PointAttribute> pntAtt = std::make_unique<draco::PointAttribute>();
			pntAtt->Init(draco::GeometryAttribute::GENERIC, nullptr, numOfCmp, geoDataType, attAcc.normalized, 0, 0);
			pntAtt->SetIdentityMapping();
			pntAtt->Reset(dc.count());
			/* WARNNING: after set attribute pntAtt won't have any ownership of PointAttribute */
			int attID = m.AddAttribute(std::move(pntAtt));
			switch (extentPointType)
			{
			case TANGENT:
				header.tangentID = attID;
				break;
			}
			m.attribute(attID)->buffer()->Write(0, &dc.packBuffer()[0], dc.size() * dc.count());
			m.SetAttributeElementType(attID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
		}
	}

	return GES_OK;
}

GE_STATE compressMesh(std::unique_ptr<draco::Mesh>& inputMesh,
	std::unique_ptr< std::vector<uint8_t> >& outputBuffer) {
	draco::Encoder edr;
	edr.SetTrackEncodedProperties(true);
	
	draco::EncoderBuffer eBuf;
	draco::Status status;

	status = edr.EncodeMeshToBuffer(*inputMesh, &eBuf);
	if (!status.ok()) {
		*GVAR.err = status.error_msg_string();
		return GES_ERR;
	}
	outputBuffer = std::make_unique< std::vector<uint8_t> >();

	size_t bufSize = eBuf.size();
	(*outputBuffer).resize(bufSize);
	uint8_t* bufPtr = (*outputBuffer).data();

	memcpy_s(bufPtr, bufSize, eBuf.data(), bufSize);
	MLOG("compressed size ", bufSize);
	MLOG("compressed points ", edr.num_encoded_points());
	MLOG("compressed faces ", edr.num_encoded_faces());
	return GES_OK;
}

GE_STATE setDracoCompressPrimitive(tinygltf::Primitive& targetPri, tinygltf::Model& targetMod,
	const EncodedMeshBufferDesc& desc, 
	std::unique_ptr<std::vector<uint8_t> >& buf,
	std::unique_ptr<std::vector<uint32_t> >& compressIndices) {
	if (targetPri.mode != TINYGLTF_MODE_TRIANGLES) {
		(*GVAR.err) = "currently just support compress triangle primitive";
		return GES_ERR;
	}
	/* update attributes' accessor*/
	for (const auto& att : targetPri.attributes)
		/* WARNNING It can't tell the difference between pointing nothing or 0 bufferView */
		targetMod.accessors[att.second].bufferView = 0;
	/* update indices data */
	tinygltf_wrapper::IndexContainer ic = tinygltf_wrapper::GetIndices(targetPri, targetMod);
	tinygltf::Accessor& idxAcc = tinygltf_wrapper::IndexAccessor(targetPri, targetMod);
	std::function<bool(uint32_t, uint32_t)> writeFunc = ic.WriteFunc(idxAcc.componentType);
	for (uint32_t i = 0; i < idxAcc.count; ++i)
		if (!writeFunc(i, (*compressIndices)[i])) {
			*GVAR.err = "index's index is out of range!";
			return GES_ERR;
		}
	/* add compressed buffer */
	tinygltf::BufferView compressBufView;
	compressBufView.byteLength = buf->size();
	compressBufView.byteOffset = 0;
	compressBufView.byteStride = 0;
	compressBufView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	targetMod.buffers.push_back(tinygltf::Buffer());
	compressBufView.buffer = targetMod.buffers.size() - 1;
	tinygltf::Buffer& compressBuf = targetMod.buffers[compressBufView.buffer];
	compressBuf.data.swap(*buf.get());
	targetMod.bufferViews.push_back(compressBufView);
	/* set extension */
	tinygltf::Value::Object khr_draco_mesh_compression;
	tinygltf::Value::Object ext_atts;
	/* it must have position data */
	ext_atts.insert(std::make_pair("POSITION", tinygltf::Value(static_cast<int>(desc.positionID))));
	if (desc.normalID != 0xffu) ext_atts.insert(std::make_pair("NORMAL", tinygltf::Value(static_cast<int>(desc.normalID))));
	if (desc.texcoordID != 0xffu) ext_atts.insert(std::make_pair("TEXCOORD_0", tinygltf::Value(static_cast<int>(desc.texcoordID))));
	if (desc.tangentID != 0xffu) ext_atts.insert(std::make_pair("TANGENT", tinygltf::Value(static_cast<int>(desc.tangentID))));
	if (desc.colorID != 0xffu) ext_atts.insert(std::make_pair("COLOR", tinygltf::Value(static_cast<int>(desc.colorID))));

	khr_draco_mesh_compression.insert(std::make_pair("bufferView", tinygltf::Value(static_cast<int>(targetMod.bufferViews.size() - 1))));
	khr_draco_mesh_compression.insert(std::make_pair("attributes", ext_atts));
	
	return GES_OK;
}

/* after adding compress data. we need to remove uncompress data */
/* reorganize bufferViews and buffers */
GE_STATE updateGLTFData(tinygltf::Model& model) {
	/* find out which bufferViews need to be removed */
	/* store bufferViews still being used */
	std::vector<tinygltf::BufferView> nBufferViews;
	/* store bufferViews new index */
	std::map<int, int> nBufferViewMap;
	for (auto& acc : model.accessors) {
		int bfv = acc.bufferView;
		const auto& iter = nBufferViewMap.find(bfv);
		if (iter == nBufferViewMap.end()) {
			/* need to copy this bufferView to nBufferViews */
			nBufferViews.push_back(model.bufferViews[bfv]);
			nBufferViewMap.insert(std::make_pair(bfv, nBufferViews.size() - 1));
			acc.bufferView = nBufferViews.size() - 1;
		}
		else {
			/* update accessor's bufferView */
			acc.bufferView = iter->second;
		}
	}
	model.bufferViews.swap(nBufferViews);
	/* remove buffers no longer used */
	/* when combine buffer data, be careful about the buffer alignment */
	
}

/*
foreach mesh
	foreach primitive
		if primitive is triangle then
			compress result = compress primitive
			delete primitive attributes accessors
			modify primitive indices
			add buffer conatin compress result
			reconstruct buffers
		end if
	end for
end for
*/
