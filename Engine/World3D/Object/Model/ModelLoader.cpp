#include "ModelLoader.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include "Graphics/Resource/TextureManager.h"
#include "Utility/Logger.h"
#include "GltfAxis.h"
#include "Animation/AnimationClipSet.h"

namespace {
	// mtl(や glTF/FBX のマテリアル)からKa/Kd/Ks/Ns/Ni/dを読み取る。
	// 項目が無いモデルもあるので、取得できなかったものはMaterialDataの既定値のままにする。
	void ReadMaterialParameters(const aiMaterial* material, MaterialData& matData) {
		aiColor3D color;
		if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
			matData.Ka = {color.r, color.g, color.b};
		}
		if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
			matData.Kd = {color.r, color.g, color.b};
		}
		if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
			matData.Ks = {color.r, color.g, color.b};
		}
		float value = 0.0f;
		if (material->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
			matData.Ns = value;
		}
		if (material->Get(AI_MATKEY_REFRACTI, value) == AI_SUCCESS) {
			matData.Ni = value;
		}
		if (material->Get(AI_MATKEY_OPACITY, value) == AI_SUCCESS) {
			matData.d = value;
		}
	}

	// 拡散色をmtlから採用してよいマテリアルの判定。
	// AssimpはKdの記述が無いmtlにも既定色(0.6のグレー)を返すため、そのまま使うと
	// これまで白かったモデル(Kdを書いていないmtl)が勝手に暗くなってしまう。
	// そこでobjの場合だけ、mtlに実際にKdが書かれているマテリアルに限定する。
	struct DiffuseColorFilter {
		bool restrictToExplicit = false;	// objのときのみtrue
		std::set<std::string> explicitNames;

		bool Allows(const std::string& materialName) const {
			return !restrictToExplicit || explicitNames.contains(materialName);
		}
	};

	// mtlを走査して「Kd」が明記されているマテリアル名を集める
	void CollectExplicitKdNames(const std::filesystem::path& mtlPath, std::set<std::string>& names) {
		std::ifstream file(mtlPath);
		if (!file) {
			return;
		}
		std::string line;
		std::string currentName;
		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string token;
			if (!(iss >> token)) {
				continue;
			}
			if (token == "newmtl") {
				// マテリアル名は空白を含み得るので行末まで取る
				iss >> std::ws;
				std::getline(iss, currentName);
				while (!currentName.empty() && (currentName.back() == '\r' || currentName.back() == ' ')) {
					currentName.pop_back();
				}
			} else if (token == "Kd" && !currentName.empty()) {
				names.insert(currentName);
			}
		}
	}

	// objが参照しているmtlを調べてフィルタを作る（obj以外はAssimpの値をそのまま信用する）
	DiffuseColorFilter MakeDiffuseColorFilter(const std::string& filePath) {
		DiffuseColorFilter filter;
		std::filesystem::path objPath(filePath);
		if (objPath.extension() != ".obj") {
			return filter;
		}
		filter.restrictToExplicit = true;

		// mtllib の指定を探す。頂点データが始まったらそれ以降には無いので打ち切る
		bool foundMtllib = false;
		std::ifstream objFile(objPath);
		std::string line;
		while (std::getline(objFile, line)) {
			std::istringstream iss(line);
			std::string token;
			if (!(iss >> token)) {
				continue;
			}
			if (token == "mtllib") {
				std::string mtlName;
				iss >> std::ws;
				std::getline(iss, mtlName);
				while (!mtlName.empty() && (mtlName.back() == '\r' || mtlName.back() == ' ')) {
					mtlName.pop_back();
				}
				if (!mtlName.empty()) {
					CollectExplicitKdNames(objPath.parent_path() / mtlName, filter.explicitNames);
					foundMtllib = true;
				}
			} else if (token == "v" || token == "f") {
				break;
			}
		}
		// mtllibが書かれていないobjのために <モデル名>.mtl も見ておく
		if (!foundMtllib) {
			std::filesystem::path fallback = objPath;
			fallback.replace_extension(".mtl");
			CollectExplicitKdNames(fallback, filter.explicitNames);
		}
		return filter;
	}

	// シェーダーに渡す基本色を決める。
	// テクスチャ付きマテリアルはBlenderが既定で Kd 0.8 を書き出してしまい、
	// 掛けるとテクスチャが暗くなるだけなので白のままにする。
	// テクスチャを持たないマテリアルはKdが唯一の色情報なのでそれを採用する。
	void ResolveBaseColor(MaterialData& matData, const DiffuseColorFilter& filter) {
		if (matData.hasTexture || !filter.Allows(matData.name)) {
			matData.baseColor = {1.0f, 1.0f, 1.0f, matData.d};
		} else {
			matData.baseColor = {matData.Kd.r, matData.Kd.g, matData.Kd.b, matData.d};
		}
	}

}

// 宣言は ModelLoader.h。.anim.json など付随ファイルからも使うので公開している
std::string ModelLoader::MakeAssetBasePath(const std::string& modelName) {
	const size_t separator = modelName.find_last_of("/\\");
	const std::string stem = (separator == std::string::npos) ? modelName : modelName.substr(separator + 1);
	return "Resource/Models/" + modelName + "/" + stem;
}

void ModelLoader::Initialize(DirectXManager* dxManager, SrvManager* srvManager) {
	dxManager_ = dxManager;
	srvManager_ = srvManager;
}

ModelData ModelLoader::LoadModelFile(const std::string& filename) {
	ModelData modelData;

	Assimp::Importer importer;
	// 拡張子を自動判別する（.obj が無ければ .gltf → .fbx の順に探す）
	const std::string basePath = ModelLoader::MakeAssetBasePath(filename);
	std::string filePath = basePath + ".obj";
	if (!std::filesystem::exists(filePath)) {
		for (const char* ext : {".gltf", ".fbx"}) {
			if (std::filesystem::exists(basePath + ext)) {
				filePath = basePath + ext;
				break;
			}
		}
	}

	Logger::Log("[ModelLoader] Loading: " + filePath);
	// Triangulate: FBXなどに含まれる5角形以上の面も三角形化して取りこぼさないようにする
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs | aiProcess_Triangulate);
	ASSERT_MSG(scene && scene->HasMeshes(), ("[ModelLoader] モデルの読み込みに失敗しました。\n  パス: " + filePath + "\n  Assimp: " + importer.GetErrorString()).c_str());

	// --- マテリアルの読み込み ---
	const DiffuseColorFilter diffuseFilter = MakeDiffuseColorFilter(filePath);
	modelData.materials.resize(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; ++i) {
		aiMaterial* material = scene->mMaterials[i];
		MaterialData matData;
		matData.name = material->GetName().C_Str();
		ReadMaterialParameters(material, matData);

		std::string texPath;
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			texPath = textureFilePath.C_Str();

			// 壊れたマテリアル対策: 空・"." や Resource/Images に実在しないテクスチャは白にフォールバックする
			// （Blenderが画像を持たないテクスチャノードを "map_Kd ." として書き出すことがある）
			if (texPath.empty() || !std::filesystem::is_regular_file("Resource/Images/" + texPath)) {
				Logger::Log("[ModelLoader] 無効なテクスチャパスのため white.png を使用します: \"" + texPath + "\" (" + filename + ")\n");
				texPath.clear();
			}
		}
		matData.hasTexture = !texPath.empty();
		if (texPath.empty()) {
			texPath = "white.png";
		}
		// テクスチャが無い場合はmtlのKdを色として使う
		ResolveBaseColor(matData, diffuseFilter);

		matData.textureFilePath = texPath;
		TextureManager::GetInstance().LoadTexture(matData.textureFilePath);
		matData.textureIndex = TextureManager::GetInstance().GetTextureIndexByFilePath(matData.textureFilePath);

		modelData.materials[i] = matData;
	}

	// --- メッシュの読み込み ---
	modelData.meshes.resize(scene->mNumMeshes);
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		MeshData meshData;
		aiMesh* mesh = scene->mMeshes[meshIndex];

		ASSERT_MSG(mesh->HasNormals(), ("[ModelLoader] メッシュに法線がありません: " + std::string(mesh->mName.C_Str())).c_str());
		// UVを持たないモデル（FBXの壁モデルなど）はダミーUVで読み込む
		const bool hasUV = mesh->HasTextureCoords(0);

		meshData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& pos = mesh->mVertices[vertexIndex];
			aiVector3D& norm = mesh->mNormals[vertexIndex];

			meshData.vertices[vertexIndex].position = {-pos.x, pos.y, pos.z, 1.0f};
			meshData.vertices[vertexIndex].normal = {-norm.x, norm.y, norm.z};
			if (hasUV) {
				aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];
				meshData.vertices[vertexIndex].texcoord = {uv.x, uv.y};
			} else {
				meshData.vertices[vertexIndex].texcoord = {0.0f, 0.0f};
			}
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices == 4) {
				int32_t i0 = face.mIndices[0];
				int32_t i1 = face.mIndices[1];
				int32_t i2 = face.mIndices[2];
				int32_t i3 = face.mIndices[3];
				meshData.indices.insert(meshData.indices.end(), {i0, i1, i2, i0, i2, i3});
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

SkinnedModelData ModelLoader::LoadSkinnedModel(const std::string& filename, AnimationClipSet* outClips) {
	SkinnedModelData modelData;

	Assimp::Importer importer;

	std::string filePath = ModelLoader::MakeAssetBasePath(filename) + ".gltf";

	Logger::Log("[ModelLoader] Loading skinned: " + filePath);
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	ASSERT_MSG(scene && scene->HasMeshes(),
		("[ModelLoader] スキンモデルの読み込みに失敗しました。\n  パス: " + filePath + "\n  Assimp: " + importer.GetErrorString()).c_str());

	// 同じ scene からアニメーションも取る。ここで取らないと gltf をもう一度開くことになる
	if (outClips) {
		outClips->LoadFromScene(scene);
	}

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
		matData.name = material->GetName().C_Str();

		if (matData.name == "") {
			break;
		}
		ReadMaterialParameters(material, matData);

		std::string texPath;
		if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			texPath = textureFilePath.C_Str();

			// 壊れたマテリアル対策: 空・"." や Resource/Images に実在しないテクスチャは白にフォールバックする
			if (texPath.empty() || !std::filesystem::is_regular_file("Resource/Images/" + texPath)) {
				Logger::Log("[ModelLoader] 無効なテクスチャパスのため白テクスチャを使用します: \"" + texPath + "\" (" + filename + ")\n");
				texPath.clear();
			}
		}

		matData.hasTexture = !texPath.empty();
		// テクスチャが無い場合はマテリアルのKdを色として使う（gltfなので制限なし）
		ResolveBaseColor(matData, DiffuseColorFilter{});

		if (!texPath.empty()) {
			matData.textureFilePath = texPath;
			TextureManager::GetInstance().LoadTexture(matData.textureFilePath);
			matData.textureIndex = TextureManager::GetInstance().GetTextureIndexByFilePath(matData.textureFilePath);
		} else {
			// テクスチャが無い場合は白テクスチャを使用
			matData.textureFilePath = "__WHITE__"; // ログなどのデバッグ用
			matData.textureIndex = TextureManager::GetInstance().GetWhiteTextureIndex();
		}

		modelData.materials[i] = matData;
	}

	// --- メッシュの読み込み ---
	modelData.meshes.resize(scene->mNumMeshes);
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		ASSERT_MSG(mesh->HasNormals(), ("[ModelLoader] スキンメッシュに法線がありません: " + std::string(mesh->mName.C_Str())).c_str());
		bool hasUV = mesh->HasTextureCoords(0);  // UVの有無を確認

		SkinnedMeshData SkinnedMeshData;
		SkinnedMeshData.skinClusterName = mesh->mName.C_Str();
		SkinnedMeshData.vertices.resize(mesh->mNumVertices);
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {
			aiVector3D& pos = mesh->mVertices[vertexIndex];
			aiVector3D& norm = mesh->mNormals[vertexIndex];

			const Vector3 position = GltfAxis::Position(pos);
			SkinnedMeshData.vertices[vertexIndex].position = {position.x, position.y, position.z, 1.0f};
			SkinnedMeshData.vertices[vertexIndex].normal = GltfAxis::Normal(norm);

			if (hasUV) {
				aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];
				SkinnedMeshData.vertices[vertexIndex].texcoord = {uv.x, uv.y};
			} else {
				SkinnedMeshData.vertices[vertexIndex].texcoord = {0.0f, 0.0f}; // ダミー
			}
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			if (face.mNumIndices == 4) {
				int32_t i0 = face.mIndices[0];
				int32_t i1 = face.mIndices[1];
				int32_t i2 = face.mIndices[2];
				int32_t i3 = face.mIndices[3];
				SkinnedMeshData.indices.insert(SkinnedMeshData.indices.end(), {i0, i1, i2, i0, i2, i3});
			} else if (face.mNumIndices == 3) {
				for (uint32_t i = 0; i < 3; ++i) {
					SkinnedMeshData.indices.push_back(face.mIndices[i]);
				}
			}
		}

		// --- ボーン情報の抽出 ---
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = SkinnedMeshData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);

			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				GltfAxis::Scale(scale),
				GltfAxis::Rotation(rotate),
				GltfAxis::Position(translate)
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
		SkinnedMeshData.materialIndex = mesh->mMaterialIndex;

		modelData.meshes[meshIndex] = SkinnedMeshData;
	}

	return modelData;
}

Node ModelLoader::ReadNode(aiNode* node) {
	Node result;
	aiVector3D scale, translate;
	aiQuaternion rotate;
	node->mTransformation.Decompose(scale, rotate, translate); // assimpの行列からSRTを抽出する関数
	result.transform.scale = GltfAxis::Scale(scale);
	result.transform.rotate = GltfAxis::Rotation(rotate);
	result.transform.translate = GltfAxis::Position(translate);
	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

	result.name = node->mName.C_Str(); // node名を格納
	result.children.resize(node->mNumChildren); // 子供の数だけ確保
	for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
		// 再帰的に読むことで階層構造を作る
		result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
	}

	return result;
}
