#define UTILITY_IMPLEMENTATION
#include "Utility.h"
#include <tinygltf/tiny_gltf.h>
using namespace GLTF_ENCODER;

void DracoMeshToOBJ(draco::Mesh& mesh, const char* outputName) {
	const draco::PointAttribute* pos = mesh.GetNamedAttribute(draco::GeometryAttribute::POSITION);
	if (!pos) return;
	// for normal
	const draco::PointAttribute* nor = mesh.GetNamedAttribute(draco::GeometryAttribute::NORMAL);

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
		if (nor) {
			nor->GetMappedValue(i, vec3);
			op.SetNormal(i.value(), vec3[0], vec3[1], vec3[2]);
		}
	}
	if (!op.Finalize(outputName))
		Test_Log(op.err_msg());
}

void GLTFMeshToOBJ(tinygltf::Model& m, tinygltf::Primitive& pte, const char* name) {
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

	for (size_t i = 0; i < numOfFaces; ++i) {
		uint32_t idx[3] = { 0 };
		memcpy_s(&idx[0], idxSize, data, idxSize); data += idxSize;
		memcpy_s(&idx[1], idxSize, data, idxSize); data += idxSize;
		memcpy_s(&idx[2], idxSize, data, idxSize); data += idxSize;
		oh.SetFace(i, idx[0], idx[1], idx[2]);
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
	for (size_t i = 0; i < numOfPoints; ++i) {
		float pos[3];
		memcpy_s(pos, 12, data, 12);
		oh.SetPosition(i, pos[0], pos[1], pos[2]);
		data += 12 + byteStride;
	}
	oh.Finalize(name);
}

inline size_t indexAccessorID(tinygltf::Model& m, draco::Mesh& mesh) {
	m.buffers.push_back(tinygltf::Buffer());
	size_t idxBufID = m.buffers.size() - 1;
	size_t bufSize = mesh.num_faces * sizeof(draco::Mesh::Face);
	m.buffers[idxBufID].data.resize(bufSize);
	uint8_t* ptr = m.buffers[idxBufID].data.data();
	for (draco::FaceIndex i(0); i < mesh.num_faces; ++i) {
		const draco::Mesh::Face& face = mesh.face(i);
		memcpy_s(ptr, sizeof(face), &face, sizeof(face));
		ptr += sizeof(face);
	}

	tinygltf::BufferView bufView;
	bufView.buffer = idxBufID;
	bufView.byteLength = bufSize;
	bufView.byteOffset = 0;
	bufView.byteStride = 0;
	bufView.target = TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER;
	m.bufferViews.push_back(bufView);
	size_t idxBufViewID = m.bufferViews.size() - 1;

	tinygltf::Accessor acc;
	acc.bufferView = idxBufViewID;
	acc.byteOffset = 0;
	/* since draco PointIndex is uint32_t */
	acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
	acc.count = mesh.num_faces * 3;
	acc.maxValues.push_back(mesh.num_points() - 1);
	acc.minValues.push_back(0);
	acc.normalized = false;
	acc.type = TINYGLTF_TYPE_SCALAR;
	m.accessors.push_back(acc);
	return m.accessors.size() - 1;
}

inline size_t attributeAccessorID(tinygltf::Model& m, draco::Mesh& mesh, uint32_t attID) {
	const draco::PointAttribute* attPtr = mesh.attribute(attID);
	m.buffers.push_back(tinygltf::Buffer());
	size_t bufID = m.buffers.size() - 1;
	size_t bufSize = attPtr->buffer()->data_size();
	m.buffers[bufID].data.resize(bufSize);
	attPtr->buffer()->Read(0, m.buffers[bufID].data.data(), bufSize);

	tinygltf::BufferView bufView;
	bufView.buffer = bufID;
	bufView.byteLength = bufSize;
	bufView.byteOffset = bufView.byteStride = 0;
	bufView.target = TINYGLTF_TARGET_ARRAY_BUFFER;
	m.bufferViews.push_back(bufView);

	tinygltf::Accessor acc;
	acc.bufferView = m.bufferViews.size() - 1;
	acc.byteOffset = 0;
	switch (attPtr->data_type()) {
	case draco::DataType::DT_INT8:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_BYTE; break;
	case draco::DataType::DT_UINT8:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE; break;
	case draco::DataType::DT_INT16:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_SHORT; break;
	case draco::DataType::DT_UINT16:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT; break;
	case draco::DataType::DT_UINT32:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT; break;
	case draco::DataType::DT_FLOAT32:
		acc.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT; break;
	default:
		assert(false);
		break;
	}
	acc.count = mesh.num_points();
	acc.normalized = false;
	switch (attPtr->num_components())
	{
	case 1:
		acc.type = TINYGLTF_TYPE_SCALAR; break;
	case 2:
		acc.type = TINYGLTF_TYPE_VEC2; break;
	case 3:
		acc.type = TINYGLTF_TYPE_VEC3; break;
	case 4:
		acc.type = TINYGLTF_TYPE_VEC4; break;
	default:
		assert(false);
		break;
	}
	m.accessors.push_back(acc);
	return m.accessors.size() - 1;
}

void DracoMeshToGLTFMesh(tinygltf::Model& m, tinygltf::Mesh& outputMesh,
	draco::Mesh& mesh, const GLTF_ENCODER::MeshBufferHeader& header) {
	/* There must be position data */
	if (header.positionID == 0xffu) {
		Test_Log("no position data!");
		return;
	}
	/* TODO: currently ignore material. */
	/* Here assume mesh's attribute data was not deduplicated */
	tinygltf::Primitive pri;
	pri.mode = TINYGLTF_MODE_TRIANGLES;
	/* process indices */
	pri.indices = indexAccessorID(m, mesh);
	/* process position */
	pri.attributes.insert(std::make_pair(std::string("POSITION"),
		attributeAccessorID(m, mesh, header.positionID)));
	/* process normal */
	if (header.normalID != 0xffu) {
		pri.attributes.insert(std::make_pair(std::string("NORMAL"),
			attributeAccessorID(m, mesh, header.normalID)));
	}
	/* process texcoord */
	if (header.texcoordID != 0xffu) {
		pri.attributes.insert(std::make_pair(std::string("TEXCOORD_0"),
			attributeAccessorID(m, mesh, header.texcoordID)));
	}
	/* process tangent */
	if (header.tangentID != 0xffu) {
		pri.attributes.insert(std::make_pair(std::string("TANGENT"),
			attributeAccessorID(m, mesh, header.tangentID)));
	}
	outputMesh.primitives.push_back(pri);
}

GE_STATE decompress(const int8_t* data) {
	GE_STATE state = GES_OK;
	EncoderHeader eh;
	memcpy_s(&eh, sizeof(EncoderHeader),
		data, sizeof(EncoderHeader));
	const int8_t* dataPtr = data;
	/* decompressEachBuffer */
	dataPtr += eh.headerSize;
	char outputName[] = "decompressed0.obj";
	for (uint32_t i = 0; i < eh.numOfbuffer; ++i) {
		MeshBufferHeader bh;
		memcpy_s(&bh, sizeof(MeshBufferHeader),
			dataPtr, sizeof(MeshBufferHeader));
		state = decompressMesh(dataPtr + bh.headerSize, bh.bufferLength - sizeof(MeshBufferHeader));
		if (state != GES_OK) { return state; }
		outputName[12] = i + 'A';
		DracoMeshToOBJ(*T_GVAR.meshPtr, outputName);
		dataPtr += bh.bufferLength;
	}
	return state;
}

GE_STATE decompressMesh(
	const int8_t* data,
	size_t size) {
	draco::Decoder ddr;
	draco::DecoderBuffer deBuf;
	deBuf.Init(reinterpret_cast<const char*>(data), size);
	draco::StatusOr< std::unique_ptr<draco::Mesh> >&& rStatus
		= ddr.DecodeMeshFromBuffer(&deBuf);
	if (!rStatus.ok()) {
		Test_Log(rStatus.status().error_msg_string());
		return GES_ERR;
	}
	T_GVAR.meshPtr = std::move(rStatus).value();
	return GES_OK;
}

/* TODO validate the compress result by decompress it and recreate a gltf file */
GE_STATE analyseDecompressMesh() {
	DracoMeshToOBJ(*T_GVAR.meshPtr, "uncompressed.obj");

	/* TODO: consider there will be many meshes */
	/* you need to build model mannually */

	return GES_OK;
}
