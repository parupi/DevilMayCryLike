#include "MeshShapeSampler.h"
#include <algorithm>
#include "Math/MathUtils.h"
#include "World3D/Object/Model/ModelManager.h"

bool MeshShapeSampler::Build(const std::string& modelName)
{
	triangles_.clear();
	cumulativeAreas_.clear();
	totalArea_ = 0.0f;

	auto& modelManager = ModelManager::GetInstance();

	if (modelManager.models.contains(modelName)) {
		// 通常モデル
		ModelData data = modelManager.models.at(modelName)->GetModelData();
		for (const auto& mesh : data.meshes) {
			AddMesh(mesh.vertices, mesh.indices);
		}
	} else if (modelManager.skinnedModels.contains(modelName)) {
		// スキンモデルはバインドポーズの形状でサンプリングする
		SkinnedModelData data = modelManager.skinnedModels.at(modelName)->GetModelData();
		for (const auto& mesh : data.meshes) {
			AddMesh(mesh.vertices, mesh.indices);
		}
	}

	return IsValid();
}

void MeshShapeSampler::AddMesh(const std::vector<VertexData>& vertices, const std::vector<int32_t>& indices)
{
	const int32_t vertexCount = static_cast<int32_t>(vertices.size());

	for (size_t i = 0; i + 2 < indices.size(); i += 3) {
		const int32_t i0 = indices[i + 0];
		const int32_t i1 = indices[i + 1];
		const int32_t i2 = indices[i + 2];

		// 不正なインデックスはスキップ
		if (i0 < 0 || i1 < 0 || i2 < 0) continue;
		if (i0 >= vertexCount || i1 >= vertexCount || i2 >= vertexCount) continue;

		Triangle tri;
		const Vector4& p0 = vertices[i0].position;
		const Vector4& p1 = vertices[i1].position;
		const Vector4& p2 = vertices[i2].position;
		tri.a = { p0.x, p0.y, p0.z };
		tri.b = { p1.x, p1.y, p1.z };
		tri.c = { p2.x, p2.y, p2.z };

		// 縮退三角形（面積ほぼゼロ）はスキップ
		const float area = Length(Cross(tri.b - tri.a, tri.c - tri.a)) * 0.5f;
		if (area <= 1.0e-8f) continue;

		totalArea_ += area;
		triangles_.push_back(tri);
		cumulativeAreas_.push_back(totalArea_);
	}
}

Vector3 MeshShapeSampler::Sample(std::mt19937& randomEngine) const
{
	if (triangles_.empty()) return {};

	// 面積で重み付けして三角形を抽選する
	std::uniform_real_distribution<float> distArea(0.0f, totalArea_);
	const float r = distArea(randomEngine);

	auto it = std::lower_bound(cumulativeAreas_.begin(), cumulativeAreas_.end(), r);
	size_t index = static_cast<size_t>(std::distance(cumulativeAreas_.begin(), it));
	if (index >= triangles_.size()) index = triangles_.size() - 1;

	const Triangle& tri = triangles_[index];

	// 三角形内の一様サンプリング（バリセントリック座標）
	std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
	float u = dist01(randomEngine);
	float v = dist01(randomEngine);
	if (u + v > 1.0f) {
		u = 1.0f - u;
		v = 1.0f - v;
	}

	return tri.a + (tri.b - tri.a) * u + (tri.c - tri.a) * v;
}
