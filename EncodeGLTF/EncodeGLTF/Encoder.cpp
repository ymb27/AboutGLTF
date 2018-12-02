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
#include <set>
using std::unique_ptr;
using namespace GLTF_ENCODER;

#define TEST_COMPRESS

#ifdef TEST_COMPRESS
#include "Utility.h"
/* LOG for developing data */
#define MLOG(...) Test_Log(__VA_ARGS__ )
extern std::string GBaseDir;
#else
#define MLOG(...)
#endif /* TEST_COMPRESS */

/* global variable */
struct {
	std::string* err;
	std::string* warn;
} GVAR;

/* load model data from json data */
inline GE_STATE loadModel(const std::string& jsonData, tinygltf::Model* outputModel);
/* set explicit attribute quantization. */
/* Get max/min value and number of components refer to accessor */
/* set quantization bits, according to attribute |type| */
/* if max/min values are defined, its quantization bits is |optBit| or set to |defBit| */
/* WARNING! caller should guarantee that max/min values are correct if defined*/
inline void encoderSettingHelper(draco::Encoder& encoder, draco::GeometryAttribute::Type type,
	int defBit, int optBit, const tinygltf::Accessor& acc, int numOfCmp);
/* make a draco mesh according to the triangle primitive */
/* caller should guarantee that primitive is triangle */
inline GE_STATE makeMesh(const tinygltf::Primitive& triangleMeshPrimitive, tinygltf::Model& gltf,
	std::unique_ptr<draco::Mesh>& outputMesh, draco::Encoder& encoder,
	EncodedMeshBufferDesc& desc);
/* compress mesh maked by previous function */
inline GE_STATE compressMesh(std::unique_ptr<draco::Mesh>& inputMesh,
	draco::Encoder& encoder,
	std::unique_ptr< std::vector<uint8_t> >& outputBuffer);
/* pri is the origin primitive, this function will add extension to it */
/* and record that its bufferView ID and byteOffset property will be ignore, */
/* since it won't use uncompress data any more */
inline GE_STATE setDracoCompressPrimitive(tinygltf::Primitive& pri, tinygltf::Model& gltf,
	const EncodedMeshBufferDesc& desc, std::set<int>& ignoreBufferViewAccessorID);
/* update gltf file, update accessors, reorgnize its bufferViews and buffers */
inline GE_STATE updateGLTFData(tinygltf::Model& gltf, std::set<int>& ignoreBufferViewAccessorID);
/* append draco compressed data to Gltf buffers */
inline GE_STATE appendCompressBufferToGLTF(tinygltf::Model& gltf,
	std::vector<tinygltf::Primitive*>& pris,
	std::vector<std::unique_ptr<std::vector<uint8_t> > >& bufs);

GE_STATE Encoder::EncodeFromAsciiMemory(const std::string& jData) {
	GVAR.err = &m_err;
	GVAR.warn = &m_warn;
	GE_STATE state = GES_OK;
	tinygltf::Model gltf;
	/* load jsonData and intialize  |gltf|*/
	state = loadModel(jData, &gltf);
	if (state != GES_OK) return state;
	/* contain references to primitives that used draco compressed */
	std::vector<tinygltf::Primitive*> compressedPrimitives;
	/* contain pointers which point to compressed data */
	std::vector<std::unique_ptr<std::vector<uint8_t> > > compressedDatas;
	/* contain ID of accessor that need to ignore its bufferView and byteOffset */
	std::set<int> compressedPrimitiveOriginAccessorID;
	/* Traversal model file and compressed triangle primitive, */
	/* since KHR_draco_compress_extension only support triangle(strip) primitive */
	for (tinygltf::Mesh& mesh : gltf.meshes) {
		for (tinygltf::Primitive& pri : mesh.primitives) {
			/* TODO: support triangle strip */
			if (pri.mode == TINYGLTF_MODE_TRIANGLES) {
				compressedPrimitives.push_back(&pri);
				/* record each compressed attribute's id */
				EncodedMeshBufferDesc header;
				{
					draco::Encoder encoder;
					/* buffer that contain this primitives compressed data */
					std::unique_ptr<std::vector<uint8_t> > compressBuffer;
					/* build a mesh according to primitive's attributes */
					std::unique_ptr<draco::Mesh> mesh;
					/* setup encoder's setting while make the mesh */
					state = makeMesh(pri, gltf, mesh, encoder, header);
					if (state != GES_OK) return state;
					state = compressMesh(mesh, encoder, compressBuffer);
					if (state != GES_OK) return state;
					compressedDatas.push_back(std::move(compressBuffer));
				}
				/* update primitive data, add draco extension to it*/
				state = setDracoCompressPrimitive(pri, gltf, header, compressedPrimitiveOriginAccessorID);
				if (state != GES_OK) return state;
			}
		}
	}
	state = updateGLTFData(gltf, compressedPrimitiveOriginAccessorID);
	if (state != GES_OK) return state;
	appendCompressBufferToGLTF(gltf, compressedPrimitives, compressedDatas);
	gltf.extensionsRequired.push_back("KHR_draco_mesh_compression");
	gltf.extensionsUsed.push_back("KHR_draco_mesh_compression");

	m_outputBuf = std::make_unique<std::vector<uint8_t> >();
	tinygltf::TinyGLTF writer;
	writer.WriteGltfSceneToBuffer(&gltf, *m_outputBuf);
	
	return state;
}

/* private helper functions */
GE_STATE loadModel(const std::string& jData, tinygltf::Model* gltf) {
	/* WARNNING! If gltf use indirect path to indicate its external file */
	/* IT's very important to set base_dir correctly */
	/* or Tinygltf can't guarantee loading gltf completely */
#ifdef TEST_COMPRESS
	std::string base_dir = GBaseDir;
#else /* TEST_COMPRESS */
	std::string base_dir = "";
#endif /* TEST_COMPRESS */
	tinygltf::TinyGLTF loader;
	GE_STATE state;
	bool ret = loader.LoadASCIIFromString(gltf, GVAR.err, GVAR.warn, jData.c_str(), jData.size(), base_dir);
	state = ret ? GES_OK : GES_ERR;
	if (state != GES_OK)	return state;
	/* It's invalid to compress a gltf file without any meshes */
	if (gltf->meshes.size() <= 0) {
		*GVAR.err = "No Mesh!";
		state = GES_ERR;
	}
	return state;
}

inline void encoderSettingHelper(draco::Encoder& encoder, const draco::GeometryAttribute::Type type,
	const int defBit, const int optBit, const tinygltf::Accessor& acc, const int numOfCmp) {
	if (acc.minValues.size() != numOfCmp || acc.maxValues.size() != numOfCmp) {
		/* It seems max/min value is not correct, use default setting*/
		encoder.SetAttributeQuantization(type, defBit);
	}
	else {
		/* WARNNING! I don't promise that such quantization bit setting is right but it work! :) */
		std::vector<float> candidateRange(numOfCmp);
		std::vector<float> origin(numOfCmp);
		for (int i = 0; i < numOfCmp; ++i) {
			origin[i] = static_cast<float>(acc.minValues[i]);
			candidateRange[i] = static_cast<float>(acc.maxValues[i] - acc.minValues[i]);
		}
		float range = *std::max_element(candidateRange.begin(), candidateRange.end());
		encoder.SetAttributeExplicitQuantization(type, optBit, numOfCmp, origin.data(), range);
	}
}

GE_STATE makeMesh(const tinygltf::Primitive& pri,
	tinygltf::Model& gltf,
	std::unique_ptr<draco::Mesh>& outputMesh,
	draco::Encoder& encoder,
	EncodedMeshBufferDesc& desc) {

	size_t numOfFaces = tinygltf_wrapper::NumOfFaces(pri, gltf);
	size_t numOfPoints = tinygltf_wrapper::NumOfPoints(pri, gltf);

	outputMesh = std::make_unique<draco::Mesh>();
	draco::Mesh& m = (*outputMesh);
	m.SetNumFaces(numOfFaces);
	m.set_num_points(numOfPoints);

	enum {
		TANGENT
	} extentPointType;

	/* processing index */
	tinygltf_wrapper::IndexContainer ic = tinygltf_wrapper::GetIndices(pri, gltf);
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
		draco::DataType geoDataType = draco::DataType::DT_INVALID;
		const tinygltf::Accessor& attAcc = gltf.accessors[iter.second];
		int8_t numOfCmp = 0;
		if (!iter.first.compare("POSITION")) {
			geoAttType = draco::GeometryAttribute::POSITION;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 3;
			encoderSettingHelper(encoder, geoAttType, 16, 10, attAcc, numOfCmp);
		}
		else if (!iter.first.compare("NORMAL")) {
			geoAttType = draco::GeometryAttribute::NORMAL;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 3;
			encoderSettingHelper(encoder, geoAttType, 16, 10, attAcc, numOfCmp);
		}
		else if (!iter.first.compare("TEXCOORD_0")) {
			geoAttType = draco::GeometryAttribute::TEX_COORD;
			numOfCmp = 2;
			switch (attAcc.componentType) {
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				geoDataType = draco::DataType::DT_FLOAT32;
				encoderSettingHelper(encoder, geoAttType, 16, 10, attAcc, numOfCmp);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				geoDataType = draco::DataType::DT_UINT8;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				geoDataType = draco::DataType::DT_UINT16;
				break;
			}
		}
		else if (!iter.first.compare("COLOR_0")) {
			geoAttType = draco::GeometryAttribute::COLOR;
			switch (attAcc.type) {
			case TINYGLTF_TYPE_VEC3:
				numOfCmp = 3; break;
			case TINYGLTF_TYPE_VEC4:
				numOfCmp = 4; break;
			default:
				assert(false); break;
			}
			switch (attAcc.componentType) {
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				geoDataType = draco::DataType::DT_FLOAT32; 
				encoderSettingHelper(encoder, geoAttType, 16, 10, attAcc, numOfCmp);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				geoDataType = draco::DataType::DT_UINT8; break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				geoDataType = draco::DataType::DT_UINT16; break;
			default:
				assert(false); break;
			}
		}
		else if (!iter.first.compare("TANGENT")) {
			geoAttType = draco::GeometryAttribute::GENERIC;
			geoDataType = draco::DataType::DT_FLOAT32;
			numOfCmp = 4;
			extentPointType = TANGENT;
		}
		else {
			/* TODO: currently doesn't support texcoord_1, joints_0, weights_0 */
			MLOG("currently doesn't support", iter.first);
			continue;
		}
		tinygltf_wrapper::DataContainer dc = tinygltf_wrapper::DataContainer::Create(attAcc, gltf);
		std::unique_ptr<draco::PointAttribute> pntAtt = std::make_unique<draco::PointAttribute>();
		pntAtt->Init(geoAttType, nullptr, numOfCmp, geoDataType, attAcc.normalized, 0, 0);
		pntAtt->SetIdentityMapping();
		pntAtt->Reset(dc.count());
		/* WARNNING: after set attribute pntAtt won't have any ownership of PointAttribute */
		int attID = m.AddAttribute(std::move(pntAtt));
		if (geoAttType != draco::GeometryAttribute::GENERIC) {
			switch (geoAttType)
			{
			case draco::GeometryAttribute::POSITION:
				desc.positionID = attID;
				break;
			case draco::GeometryAttribute::NORMAL:
				desc.normalID = attID;
				break;
			case draco::GeometryAttribute::COLOR:
				desc.colorID = attID;
				break;
			case draco::GeometryAttribute::TEX_COORD:
				desc.texcoord0ID = attID;
				break;
			}
		}
		else {
			switch (extentPointType)
			{
			case TANGENT:
				desc.tangentID = attID;
				break;
			}
		}
		m.attribute(attID)->buffer()->Write(0, &dc.packBuffer()[0], dc.size() * dc.count());
		m.SetAttributeElementType(attID, draco::MeshAttributeElementType::MESH_VERTEX_ATTRIBUTE);
	}

	return GES_OK;
}

GE_STATE compressMesh(std::unique_ptr<draco::Mesh>& inputMesh,
	draco::Encoder& encoder,
	std::unique_ptr< std::vector<uint8_t> >& outputBuffer) {
	/* General Setting */
	encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, 12);
#ifdef TEST_COMPRESS
	encoder.SetTrackEncodedProperties(true);
#endif /* TEST_COMPRESS */

	draco::EncoderBuffer eBuf;
	draco::Status status;

	status = encoder.EncodeMeshToBuffer(*inputMesh, &eBuf);
	if (!status.ok()) {
		*GVAR.err = status.error_msg_string();
		return GES_ERR;
	}
	outputBuffer = std::make_unique< std::vector<uint8_t> >();

	size_t bufSize = eBuf.size();
	(*outputBuffer).resize(bufSize);
	uint8_t* bufPtr = (*outputBuffer).data();

	memcpy(bufPtr, eBuf.data(), bufSize);
	MLOG("compressed size ", bufSize);
	MLOG("compressed points ", encoder.num_encoded_points());
	MLOG("compressed faces ", encoder.num_encoded_faces());
	return GES_OK;
}

GE_STATE setDracoCompressPrimitive(tinygltf::Primitive& pri, tinygltf::Model& gltf,
	const EncodedMeshBufferDesc& desc, std::set<int>& ignoreBufferViewAccessorID) {
	if (pri.mode != TINYGLTF_MODE_TRIANGLES) {
		(*GVAR.err) = "currently just support compress triangle primitive";
		return GES_ERR;
	}
	/* update attributes' accessor*/
	for (const auto& att : pri.attributes)
		/* record that this attribute accessor won't need */
		/* original bufferView(bufferData) any more */
		ignoreBufferViewAccessorID.insert(att.second);
	/* record that original index data is deprecated */
	ignoreBufferViewAccessorID.insert(pri.indices);

	/* setup extension */
	tinygltf::Value::Object extObj;
	tinygltf::Value::Object khr_draco_mesh_compression;
	tinygltf::Value::Object ext_atts;
	/* it must have position data */
	ext_atts.insert(std::make_pair("POSITION", tinygltf::Value(static_cast<int>(desc.positionID))));
	if (desc.normalID != 0xffu) ext_atts.insert(std::make_pair("NORMAL", tinygltf::Value(static_cast<int>(desc.normalID))));
	if (desc.texcoord0ID != 0xffu) ext_atts.insert(std::make_pair("TEXCOORD_0", tinygltf::Value(static_cast<int>(desc.texcoord0ID))));
	if (desc.tangentID != 0xffu) ext_atts.insert(std::make_pair("TANGENT", tinygltf::Value(static_cast<int>(desc.tangentID))));
	if (desc.colorID != 0xffu) ext_atts.insert(std::make_pair("COLOR", tinygltf::Value(static_cast<int>(desc.colorID))));

	khr_draco_mesh_compression.insert(std::make_pair("bufferView", tinygltf::Value(static_cast<int>(gltf.bufferViews.size() - 1))));
	khr_draco_mesh_compression.insert(std::make_pair("attributes", ext_atts));
	
	extObj.insert(std::make_pair("KHR_draco_mesh_compression", tinygltf::Value(khr_draco_mesh_compression)));
	pri.extensions = tinygltf::Value(extObj);
	return GES_OK;
}

/* traverse object that need to use bufferView. if an object still use a bufferView */
/* (Its bufferView property will greater than or equal to 0) */
/* and such bufferView has been copied into |nBufferViews|, */
/* or push back such bufferView into |nBufferViews|. Using its new index */
/* to replace accessor's original bufferView property. Recording */
/* relationship between original index and new index in |nBufferViewMap| */
template<typename ObjectContainer>
inline void updateBufferViews(ObjectContainer& container,
	std::vector<tinygltf::BufferView>& nBufferViews,
	std::map<int, int>& nBufferViewMap,
	tinygltf::Model& model) {
	for (auto& obj : container) {
		/* bufferView = -1 means it doesn't need bufferView */
		if (obj.bufferView == -1) continue;
		int bfv = obj.bufferView;
		const auto& iter = nBufferViewMap.find(bfv);
		if (iter == nBufferViewMap.end()) {
			/* need to copy this bufferView to nBufferViews */
			nBufferViews.push_back(model.bufferViews[bfv]);
			nBufferViewMap.insert(std::make_pair(bfv, nBufferViews.size() - 1));
			obj.bufferView = nBufferViews.size() - 1;
		}
		else {
			/* update accessor's bufferView */
			obj.bufferView = iter->second;
		}
	}
}

GE_STATE updateGLTFData(tinygltf::Model& model, std::set<int>& ignoreBufferViewAccessorID) {
	for (int accID : ignoreBufferViewAccessorID) {
		/* set bufferView to -1, and tinygltf won't serialize this property */
		model.accessors[accID].bufferView = -1;
		/* set accessors's byteOffset to -1, and tinygltf won't serialize this property */
		model.accessors[accID].byteOffset = 0;
	}
	/* find out which bufferViews need to be removed */
	/* store bufferViews still being used */
	std::vector<tinygltf::BufferView> nBufferViews;
	/* store the relationship between original BufferViewID and new BufferViewID */
	std::map<int, int> nBufferViewMap;

	updateBufferViews(model.images, nBufferViews, nBufferViewMap, model);
	updateBufferViews(model.accessors, nBufferViews, nBufferViewMap, model);

	model.bufferViews.swap(nBufferViews);
	/* remove buffers no longer used */
	/* when combine buffer data, be careful about the buffer alignment */
	/* but now i just split them */
	std::vector<tinygltf::Buffer> nBuffers;
	/* reserve the first buffer for GLB buffer */
	nBuffers.push_back(tinygltf::Buffer());
	for (auto& bufView : model.bufferViews) {
		nBuffers.push_back(tinygltf::Buffer());
		bufView.buffer = nBuffers.size() - 1;
		tinygltf::Buffer& nBuf = nBuffers[bufView.buffer];
		nBuf.data.resize(bufView.byteLength);
		uint8_t* nDataPtr = nBuf.data.data();

		tinygltf::Buffer& buf = model.buffers[bufView.buffer];
		const uint8_t* dataPtr = buf.data.data();
		memcpy(nDataPtr, dataPtr + bufView.byteOffset, bufView.byteLength);
		bufView.byteOffset = 0;
	}
	model.buffers.swap(nBuffers);
	return GES_OK;
}

inline GE_STATE appendCompressBufferToGLTF(tinygltf::Model& gltf,
	std::vector<tinygltf::Primitive*>& pris,
	std::vector<std::unique_ptr<std::vector<uint8_t> > >& bufs) {
	/* cout the total size of memory storing all compressed data */
	size_t compressedBufSize = 0;
	for (const auto& compressedBufs : bufs)
		compressedBufSize += compressedBufs->size();
	gltf.buffers[0].data.resize(compressedBufSize);

	/* current glb buffer offset */
	size_t currentByteOffset = 0;
	for (size_t i = 0; i < pris.size(); ++i) {
		tinygltf::Primitive* p = pris[i];
		std::unique_ptr<std::vector<uint8_t> > buf = std::move(bufs[i]);
		/* initialize compressed buffer view*/
		tinygltf::BufferView compressBufView;
		compressBufView.byteLength = buf->size();
		compressBufView.byteOffset = currentByteOffset;
		compressBufView.buffer = 0;
		gltf.bufferViews.push_back(compressBufView);

		tinygltf::Value::Object& exts = p->extensions.Get<tinygltf::Value::Object>();
		tinygltf::Value::Object& khr_ext = exts["KHR_draco_mesh_compression"].Get<tinygltf::Value::Object>();
		khr_ext["bufferView"].Get<int>() = gltf.bufferViews.size() - 1;
		
		/* update glb buffer */
		memcpy(gltf.buffers[0].data.data() + currentByteOffset, buf->data(), buf->size());
		
		currentByteOffset += buf->size();
	}
	return GES_OK;
}
