#include "GltfDraco.h"

void DracoCompressGltf(Gltf &gltf) {

	LOG_FUN;

	for (int i = 0, il = gltf.GetMeshes()->size(); i < il; ++i) {

		tinygltf::Mesh mesh = gltf.GetMeshes()->at(i);

		for (int j = 0, jl = mesh.primitives.size(); j < jl; ++j) {

			tinygltf::Primitive primitive = mesh.primitives.at(j);

			Encoder encoder;
			MeshBuilder meshBuilder;
			draco::Mesh mesh;

			vector<unsigned char> faceBinary = ReadAccessorById(gltf.GetModel(), primitive.indices);

			vector<unsigned int> indices = CharToInt(faceBinary);
			int numFaces = indices.size() / 3;

			meshBuilder.AddFacesToMesh(&mesh, numFaces, (const int *)&indices[0]);
			// if(!AddFacesToMesh(&mesh, numFaces, indices)){
			//     LOG(ERROR) << "添加网格面索引错误";
			//     return;
			// }

			int attributeId = -1;

			map<string, int>::iterator it = primitive.attributes.begin();

			int l = primitive.attributes.size();

			while (it != primitive.attributes.end()) {

				vector<unsigned char> attributeBinary = ReadAccessorById(gltf.GetModel(), it->second);

				if (it->first == "POSITION") {

					tinygltf::Accessor accessor = gltf.GetModel()->accessors.at(it->second);
					vector<float> vertices = CharToFloat(attributeBinary);
					long numVertices = accessor.count;
					int numComponents = numberOfComponentsForType(accessor.type);
					attributeId = meshBuilder.AddFloatAttribute(&mesh, draco::GeometryAttribute::POSITION, numVertices, numComponents, &vertices[0]);

					if (attributeId == -1) {
						LOG(ERROR) << "添加网格点属性错误";
						it++;
						continue;
					}
				}
				it++;
			}

			//压缩默认属性设置
			encoder.SetSpeedOptions(3, 3);

			encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 12);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, 8);
			encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, 12);

			encoder.SetTrackEncodedProperties(true);

			DracoInt8Array dracoBuffer;

			int bufferLen = encoder.EncodeMeshToDracoBuffer(&mesh, &dracoBuffer);

			gltf.AddDracoPrimitive(i, j, attributeId, &dracoBuffer.values_);
		};
	};

	gltf.GetModel()->extensionsRequired.push_back("KHR_draco_mesh_compression");

	gltf.GetModel()->extensionsUsed.push_back("KHR_draco_mesh_compression");

	RemoveUnusedElements(gltf);

	gltf.MergeBuffers(gltf.GetScene().name);
}

vector<unsigned char> ReadAccessorById(tinygltf::Model *model, int accessorId) {

	tinygltf::Accessor accessor = model->accessors.at(accessorId);

	tinygltf::BufferView bufferview = model->bufferViews.at(accessor.bufferView);

	const int byteOffset = bufferview.byteOffset;

	const int byteLen = bufferview.byteLength;

	tinygltf::Buffer buffer = model->buffers.at(bufferview.buffer);

	vector<unsigned char> result;

	result.assign(buffer.data.begin() + byteOffset, buffer.data.begin() + byteOffset + byteLen);

	return result;
}

bool AddFacesToMesh(draco::Mesh *mesh, long numFaces, vector<unsigned int> faces) {
	if (!mesh)
		return false;

	mesh->SetNumFaces(numFaces);

	for (draco::FaceIndex i(0); i < numFaces; ++i) {
		draco::Mesh::Face face;
		face[0] = faces.at(i.value() * 3);
		face[1] = faces.at(i.value() * 3 + 1);
		face[2] = faces.at(i.value() * 3 + 2);

		mesh->SetFace(i, face);
	}

	return true;
}

template <typename DataTypeT>
int AddAttribute(draco::PointCloud *pc, draco::GeometryAttribute::Type type, long numVertices, long numComponents, const DataTypeT *data, draco::DataType dataType) {

	if (!pc)
		return -1;

	draco::PointAttribute att;
	att.Init(type, NULL, numComponents, dataType,
		/* normalized*/ false,
		/* stride */ sizeof(DataTypeT) * numComponents,
		/* byte_offset */ 0);

	const int att_id = pc->AddAttribute(att, /* identity_mapping*/ true, numVertices);

	draco::PointAttribute *const att_ptr = pc->attribute(att_id);

	for (draco::PointIndex i(0); i < numVertices; ++i) {
		att_ptr->SetAttributeValue(att_ptr->mapped_index(i), &data[i.value() * numComponents]);
	}

	if (pc->num_points() == 0) {
		pc->set_num_points(numVertices);
	}
	else if (pc->num_points() != numVertices) {
		return -1;
	}

	return att_id;
}

int numberOfComponentsForType(int type) {

	switch (type) {
	case TINYGLTF_TYPE_SCALAR:
		return 1;
		break;
	case TINYGLTF_TYPE_VEC2:
		return 2;
		break;
	case TINYGLTF_TYPE_VEC3:
		return 3;
		break;
	case TINYGLTF_TYPE_VEC4:
	case TINYGLTF_TYPE_MAT2:
		return 4;
		break;
	case TINYGLTF_TYPE_MAT3:
		return 9;
		break;
	case TINYGLTF_TYPE_MAT4:
		return 16;
		break;
	default:
		return 0;
		break;
	}
}

int EncodeMeshToDracoBuffer(draco::Encoder *encoder, draco::Mesh *mesh, vector<int8_t> *dracoBuffer) {

	if (!mesh)
		return 0;

	draco::EncoderBuffer buffer;

	if (mesh->GetNamedAttributeId(draco::GeometryAttribute::POSITION) == -1) {
		return 0;
	}
	if (!mesh->DeduplicateAttributeValues()) {
		return 0;
	}
	mesh->DeduplicatePointIds();
	if (!encoder->EncodeMeshToBuffer(*mesh, &buffer).ok()) {
		return 0;
	}

	dracoBuffer->assign(buffer.data(), buffer.data() + buffer.size());

	return buffer.size();
}