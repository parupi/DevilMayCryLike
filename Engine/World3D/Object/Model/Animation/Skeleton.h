#pragma once
#include "World3D/Object/Model/ModelStructs.h"

/// <summary>
/// ジョイント階層と、その「現在のポーズ」を持つ。
///
/// バインドポーズのスケルトンは SkinnedModel（アセット）が1つ持ち、
/// インスタンスはそれをコピーして自分のポーズを動かす。
/// コピー可能であることがインスタンス分離の前提なので、GPUリソースはここに持たせないこと。
/// </summary>
class Skeleton
{
public:
	// ノード階層からジョイントを組み立てる（アセット側で1回だけ）
	void BuildFromNode(const Node& rootNode);

	// transform から localMatrix / skeletonSpaceMatrix を作る。
	// ポーズを触ったら必ず最後に呼ぶ
	void Update();

	// --- ポーズ操作 ---
	// クリップの姿勢を適用する。チャンネルを持たないジョイントはバインドポーズへ戻る
	void ApplyClip(const AnimationData* clip, float time);

	/// <summary>
	/// マスクで指定したジョイントにだけクリップを weight の強さで重ねる。
	/// 「走りながら上半身だけ攻撃」のような部分ブレンドに使う
	/// </summary>
	void ApplyClipMasked(const AnimationData* clip, float time, const std::vector<uint8_t>& mask, float weight);

	/// <summary>
	/// rootJointName とその子孫に 1 が立ったマスクを作る。
	/// 上半身レイヤーなら背骨のジョイント名を渡す。見つからなければ false（マスクは空）
	/// </summary>
	bool BuildSubtreeMask(const std::string& rootJointName, std::vector<uint8_t>& outMask) const;
	// 現在のポーズを取り出す（ブレンド元の固定に使う）
	void CapturePose(std::vector<QuaternionTransform>& out) const;
	// src から現在のポーズへ t で補間する。t=0でsrc、t=1で現在のまま
	void BlendFromPose(const std::vector<QuaternionTransform>& src, float t);
	// バインドポーズへ戻す
	void ResetToBindPose();

	const Matrix4x4& GetJointMatrix(const std::string& name) const;
	// 見つからなければ nullptr。武器のボーン追従などはこちらを使う
	const Joint* FindJoint(const std::string& name) const;
	Joint* FindJoint(const std::string& name);

	size_t GetJointCount() const { return skeletonData_.joints.size(); }

	// 値返しにすると全ジョイント(std::string + vector を含む)を毎回コピーすることになるので参照で返す
	const SkeletonData& GetSkeletonData() const { return skeletonData_; }

	std::vector<std::pair<std::string, Matrix4x4>> GetBoneMatrices() const;

private:
	// NodeからJointを作る
	int32_t CreateJoint(const Node& node, const std::optional<int32_t>& parent, std::vector<Joint>& joints);

private:
	SkeletonData skeletonData_;
};
