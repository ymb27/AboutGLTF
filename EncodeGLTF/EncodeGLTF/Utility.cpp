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
