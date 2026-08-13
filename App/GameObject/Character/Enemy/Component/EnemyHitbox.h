#pragma once
#include "World3D/Object/Object3d.h"
#include "World3D/Object/Model/Animation/BoneAttachment.h"
#include "GameObject/Character/CharacterStructs.h"

/// <summary>
/// 見た目を持たない攻撃判定。スキンモデルのジョイントに追従する。
///
/// 武器を振って当てる敵（剣を持つ Skeleton など）と違い、噛みつき・尻尾・翼のように
/// 「体そのもので殴る」攻撃のために使う。判定の位置はアニメーションのポーズが決めるので、
/// モーションと当たり判定がずれない。
///
/// 使い方:
///   1. 敵が生成時に SetupAttachment() で体のレンダラーを渡す
///   2. 攻撃の開始時に Activate() でジョイント・大きさ・ダメージを設定
///   3. 敵の Update の最後（ポーズ更新より後）に Apply() を呼ぶ
///   4. 攻撃の終わりに Deactivate()
///
/// コライダーの実体は生成側（敵クラス）が CollisionManager に作って AddCollider する。
/// AddCollider が owner_ を入れるので、当たった側から GetDamageInfo() を引ける。
/// </summary>
class EnemyHitbox : public Object3d {
public:
	EnemyHitbox(std::string objectName) : Object3d(objectName) {}
	~EnemyHitbox() override = default;

	void Initialize() override;

	/// <summary>追従先のスキンモデルを描いているレンダラーを渡す</summary>
	void SetupAttachment(BaseRenderer* skinnedRenderer);

	/// <summary>
	/// 判定を有効にする。jointName の位置に halfExtents の箱が付く。
	///
	/// **halfExtents と offset はワールド単位**で渡すこと。
	/// このオブジェクトの親はモデルを縮小しているレンダラー（ボスなら 0.5 倍）なので、
	/// そのまま入れると指定の半分の大きさになってしまう。ここで親のスケールを打ち消す。
	/// 目安: プレイヤーのコライダーは1辺1.0（halfExtents 0.5）、ドラゴンの全高は約2.0。
	/// </summary>
	void Activate(const std::string& jointName, const Vector3& halfExtents,
		const Vector3& offset, const DamageInfo& damage);

	/// <summary>判定を無効にする</summary>
	void Deactivate();

	bool IsActive() const { return active_; }

	/// <summary>
	/// ジョイントの現在行列を取り込む。**敵のポーズ更新より後**に呼ぶこと。
	/// 当たり判定は CollisionManager が全オブジェクト更新の後に見るので、
	/// ここで入れておけば1フレーム遅れずに済む
	/// </summary>
	void Apply();

	/// <summary>この判定で与えるダメージ。攻撃ごとに違う値を入れる</summary>
	const DamageInfo& GetDamageInfo() const { return damage_; }

	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override {}
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override {}
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override {}

private:
	// コライダーの isActive を切り替える（見つからなければ何もしない）
	void SetColliderActive(bool active);

	BoneAttachment attachment_;
	BaseRenderer* skinnedRenderer_ = nullptr;
	DamageInfo damage_{};
	bool active_ = false;
};
