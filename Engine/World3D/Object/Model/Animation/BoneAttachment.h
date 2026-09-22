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

	/// <summary>
	/// ジョイントの行列を「キャラ本体のローカル空間」で取り出す。
	///
	/// Apply と違って、レンダラーのローカル行列（モデルの縮尺・足元オフセット）を畳み込み、
	/// 基底を正規化してモデルの縮尺を落とす。
	/// 追従させたいオブジェクトの親がレンダラーではなくキャラ本体の WorldTransform のとき
	/// （プレイヤーの武器のように、攻撃中は自前で動かしたいもの）はこちらを使う。
	/// ジョイントが無ければ false で、out は触らない
	/// </summary>
	bool GetSocketMatrix(Matrix4x4& out) const;

	/// <summary>
	/// キャラ本体のローカル空間 → スケルトン（モデル）空間 への変換行列。
	/// GetSocketMatrix が畳み込むレンダラーのローカル行列の逆で、モデルの縮尺も戻す。
	///
	/// IK の目標のように「キャラの足元基準で作った座標を、ボーンと同じ空間へ持っていく」ときに使う
	/// </summary>
	bool GetModelSpaceMatrix(Matrix4x4& out) const;

	// 対象がスキンモデルで、そのジョイントが実在するか
	bool IsValid() const;

	const std::string& GetJointName() const { return jointName_; }
	void SetJointName(const std::string& jointName) { jointName_ = jointName; }

private:
	SkinnedInstance* instance_ = nullptr;
	// GetSocketMatrix がレンダラーのローカル行列を引くために持つ
	BaseRenderer* renderer_ = nullptr;
	std::string jointName_;
};
