#pragma once
#include "World3D/Object/Model/ModelStructs.h"

/// <summary>
/// ボーン補正で「どの空間の軸まわりに回すか」。
///
/// Local  … ボーン自身の軸。肘を自分の軸で曲げる、手首をひねる、といった指定に向く
/// Parent … 親の軸。アニメーションのローカル回転と同じ空間なので、キーフレームに足す感覚で効く
/// Model  … モデル空間。「剣先を敵の方へ」のようにワールド寄りで決めたいときに使う
/// </summary>
enum class BoneRotationSpace {
	Local,
	Parent,
	Model,
};

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

	/// <summary>
	/// index とその子孫だけ行列を作り直す。
	/// IK のように「1つのジョイントを回しては末端の位置を見る」処理で、
	/// 毎回スケルトン全体（45ジョイント）を回さないためのもの。
	/// 親の行列は最新である前提
	/// </summary>
	void UpdateSubtree(int32_t index);

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

	// --- ボーン補正（アニメーションの上から回転を足す）---
	/// <summary>
	/// ジョイントへ補正回転を足す。space で回す軸の空間を選ぶ。
	/// ローカル回転だけを書き換えるので、呼んだ側が最後に Update() を回すこと。
	/// 見つからなければ false
	/// </summary>
	bool AddJointRotation(int32_t index, const Quaternion& delta, BoneRotationSpace space);
	bool AddJointRotation(const std::string& name, const Quaternion& delta, BoneRotationSpace space);

	/// <summary>
	/// ジョイントのスケルトン空間での回転。親をたどってローカル回転を合成する。
	/// skeletonSpaceMatrix から取り出さないのは、補正の途中で祖先を触った直後だと
	/// 行列がまだ作り直されていないため（IK の反復でも同じ理由でこちらを使う）
	/// </summary>
	Quaternion GetSkeletonSpaceRotation(int32_t index) const;

	// 見つからなければ -1
	int32_t FindJointIndex(const std::string& name) const;

	/// <summary>ジョイントのスケルトン空間での位置。範囲外なら原点</summary>
	Vector3 GetJointPosition(int32_t index) const;

	/// <summary>
	/// ポーズを直接書き換える用（IK など）。範囲外なら nullptr。
	/// 触ったら Update() か UpdateSubtree() を呼ぶこと
	/// </summary>
	QuaternionTransform* GetJointTransform(int32_t index);

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
