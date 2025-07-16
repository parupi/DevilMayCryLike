#pragma once
#include "3d/Object/Model/ModelStructs.h"


class SkinnedModel;

class Skeleton
{
public:
	// 初期化
	void Initialize(SkinnedModel* model);
	// 更新
	void Update();
	// アニメーションを適用
	void ApplyAnimation(AnimationData* current, AnimationData* prev, float time, float blendTime, float blendProgress);

	const Matrix4x4& GetJointMatrix(const std::string& name) const;
private:
	// Nodeの階層からSkeletonを作る
	void CreateSkeleton(const Node& rootNode);
	// NodeからJointを作る
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

private:
	// スケルトンデータ
	SkeletonData skeletonData_;

	// モデルデータ
	SkinnedModel* model_;

public: // ゲッター // セッター //
	SkeletonData GetSkeletonData() const { return skeletonData_; }
	std::vector<std::pair<std::string, Matrix4x4>> GetBoneMatrices() const;
};

