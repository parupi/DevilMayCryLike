#include "ModelLoader.h"
#include <cassert>
#include <base/TextureManager.h>

void ModelLoader::Initialize(DirectXManager* dxManager, SrvManager* srvManager)
{
	dxManager_ = dxManager;
	srvManager_ = srvManager;
}

ModelData ModelLoader::LoadModelFile(const std::string& filename)
{
	ModelData modelData;

	Assimp::Importer importer;
	std::string filePath = "Resource/Models/" + filename + "/" + filename + ".obj";

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene && scene->HasMeshes());

	// --- マテリアルの読み込み ---
	modelData.materials.resize(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial* material = scene->mMaterials[i];
		MaterialData matData;
		matData.name_ = material->GetName().C_Str();

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			matData.textureFilePath = textureFilePath.C_Str();
			TextureManager::GetInstance()->LoadTexture(matData.textureFilePath);
			matData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(matData.textureFilePath);
		}

		modelData.materials[i] = matData;
	}

	// --- メッシュの読み込み ---
	modelData.meshes.resize(scene->mNumMeshes);
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		MeshData meshData;
		aiMesh* mesh = scene->mMeshes[meshIndex];

		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		meshData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& pos = mesh->mVertices[vertexIndex];
			aiVector3D& norm = mesh->mNormals[vertexIndex];
			aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];

			meshData.vertices[vertexIndex].position = { -pos.x, pos.y, pos.z, 1.0f };
			meshData.vertices[vertexIndex].normal = { -norm.x, norm.y, norm.z };
			meshData.vertices[vertexIndex].texcoord = { uv.x, uv.y };
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices == 4) {
				int32_t i0 = face.mIndices[0];
				int32_t i1 = face.mIndices[1];
				int32_t i2 = face.mIndices[2];
				int32_t i3 = face.mIndices[3];
				meshData.indices.insert(meshData.indices.end(), { i0, i1, i2, i0, i2, i3 });
			} else if (face.mNumIndices == 3) {
				for (uint32_t i = 0; i < 3; ++i) {
					meshData.indices.push_back(face.mIndices[i]);
				}
			}
		}

		// --- マテリアルとの紐付け ---
		meshData.materialIndex = mesh->mMaterialIndex;

		modelData.meshes[meshIndex] = meshData;
	}

	return modelData;
}

SkinnedModelData ModelLoader::LoadSkinnedModel(const std::string& filename)
{
	SkinnedModelData modelData;

	Assimp::Importer importer;

	std::string filePath = "Resource/Models/" + filename + "/" + filename + ".gltf";

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene && scene->HasMeshes());

	modelData.rootNode = ReadNode(scene->mRootNode);

	// --- マテリアルの読み込み ---
	// 使用されているmaterialIndexの最大値を探す
	uint32_t maxMaterialIndex = 0;
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		if (mesh->mMaterialIndex > maxMaterialIndex) {
			maxMaterialIndex = mesh->mMaterialIndex;
		}
	}

	// 実際に使われている分だけ確保
	modelData.materials.resize(maxMaterialIndex + 1);

	for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial* material = scene->mMaterials[i];
		MaterialData matData;
		matData.name_ = material->GetName().C_Str();

		if (matData.name_ == "") {
			break;
		}

		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			matData.textureFilePath = textureFilePath.C_Str();
			TextureManager::GetInstance()->LoadTexture(matData.textureFilePath);
			matData.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(matData.textureFilePath);
		}

		modelData.materials[i] = matData;
	}

	// --- メッシュの読み込み ---
	modelData.meshes.resize(scene->mNumMeshes);
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		SkinnedMeshData SkinnedMeshData;
		aiMesh* mesh = scene->mMeshes[meshIndex];

		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		SkinnedMeshData.skinClusterName = mesh->mName.C_Str();

		SkinnedMeshData.meshData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& pos = mesh->mVertices[vertexIndex];
			aiVector3D& norm = mesh->mNormals[vertexIndex];
			aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];

			SkinnedMeshData.meshData.vertices[vertexIndex].position = { -pos.x, pos.y, pos.z, 1.0f };
			SkinnedMeshData.meshData.vertices[vertexIndex].normal = { -norm.x, norm.y, norm.z };
			SkinnedMeshData.meshData.vertices[vertexIndex].texcoord = { uv.x, uv.y };
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices == 4) {
				int32_t i0 = face.mIndices[0];
				int32_t i1 = face.mIndices[1];
				int32_t i2 = face.mIndices[2];
				int32_t i3 = face.mIndices[3];
				SkinnedMeshData.meshData.indices.insert(SkinnedMeshData.meshData.indices.end(), { i0, i1, i2, i0, i2, i3 });
			} else if (face.mNumIndices == 3) {
				for (uint32_t i = 0; i < 3; ++i) {
					SkinnedMeshData.meshData.indices.push_back(face.mIndices[i]);
				}
			}
		}

		// --- ボーン情報の抽出 ---
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z }
			);
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {
				jointWeightData.vertexWeights.push_back({
					bone->mWeights[weightIndex].mWeight,
					bone->mWeights[weightIndex].mVertexId
					});
			}
		}

		// --- マテリアルとの紐付け ---
		SkinnedMeshData.meshData.materialIndex = mesh->mMaterialIndex;

		modelData.meshes[meshIndex] = SkinnedMeshData;
	}

	return modelData;
}

Node ModelLoader::ReadNode(aiNode* node)
{
	Node result;
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate); // assimpの行列からSRTを抽出する関数
	result.transform.scale = { scale.x, scale.y, scale.z }; // scaleはそのまま
	result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w }; // xを反転、回転方向が逆なので軸も反転
	result.transform.translate = { -translate.x, translate.y, translate.z }; // x軸を反転
	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name_ = node->mName.C_Str(); // node名を格納
	result.children.resize(node->mNumChildren); // 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 再帰的に読むことで階層構造を作る
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

bool ModelLoader::HasBones(const aiScene* scene)
{
	// メッシュがなければボーンもない
	if (!scene->HasMeshes()) {
		return false;
	}

	// 各メッシュをチェック
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];

		// ボーン数が0でなければボーンがあると判断
		if (mesh->mNumBones > 0) {
			return true;
		}
	}

	// どのメッシュにもボーンが含まれていない場合
	return false;
}
