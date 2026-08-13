#pragma once
#include <string>
#include <Math/Matrix4x4.h>

class BaseRenderer;
class SkinnedInstance;
class WorldTransform;

/// <summary>
/// スキンモデルのジョイントに、別オブジェクトのトランスフォームを追従させる。
///
/// 使い方:
///   1. 追従させたい側の WorldTransform の親を、スキンモデルを描く ModelRenderer の
///      WorldTransform にしておく（オブジェクト本体のトランスフォームではない）
///   2. Initialize でレンダラーとジョイント名を渡す
///   3. 毎フレーム、スキンモデルの更新より後に Apply を呼ぶ
///
/// これで最終的なワールド行列は 追従側ローカル * ジョイント行列 * レンダラーのワールド になる。
/// 追従側の translation/rotation はジョイント基準のオフセットとしてそのまま使える。
/// </summary>
class BoneAttachment
{
public:
	void Initialize(BaseRenderer* skinnedRenderer, const std::string& jointName);

	// ジョイントの現在行列を target へ流し込む。ポーズ更新後に呼ぶこと
	void Apply(WorldTransform* target) const;

	// 対象がスキンモデルで、そのジョイントが実在するか
	bool IsValid() const;

	const std::string& GetJointName() const { return jointName_; }
	void SetJointName(const std::string& jointName) { jointName_ = jointName; }

private:
	SkinnedInstance* instance_ = nullptr;
	std::string jointName_;
};
