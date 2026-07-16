#pragma once
#include <string>
#include <vector>
#include <random>
#include "Math/Vector3.h"
#include "World3D/Object/Model/ModelStructs.h"

/// <summary>
/// モデルのメッシュ表面からランダムな点をサンプリングするクラス。
/// 「オブジェクトの形」にパーティクルを発生させるエミッターに使う。
/// 三角形を面積で重み付けして抽選するため、点は表面上に均一に分布する。
/// </summary>
class MeshShapeSampler
{
public:
	/// <summary>
	/// ModelManager に読み込み済みのモデル名からサンプラーを構築する。
	/// 通常モデル・スキンモデル（バインドポーズ）の両方に対応。
	/// </summary>
	/// <returns>モデルが見つからない・有効な三角形が無い場合は false</returns>
	bool Build(const std::string& modelName);

	bool IsValid() const { return !triangles_.empty(); }

	/// <summary>メッシュ表面上のランダムな点（モデルローカル座標）を返す</summary>
	Vector3 Sample(std::mt19937& randomEngine) const;

private:
	struct Triangle {
		Vector3 a, b, c;
	};

	// メッシュ1つ分の三角形を面積テーブルに追加する
	void AddMesh(const std::vector<VertexData>& vertices, const std::vector<int32_t>& indices);

	std::vector<Triangle> triangles_;
	std::vector<float> cumulativeAreas_; // 面積の累積和（面積重み付き抽選用）
	float totalArea_ = 0.0f;
};
