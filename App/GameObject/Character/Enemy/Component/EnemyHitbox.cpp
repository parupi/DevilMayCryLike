#include "EnemyHitbox.h"
#include "World3D/Collider/OBBCollider.h"
#include "World3D/Object/Renderer/BaseRenderer.h"

void EnemyHitbox::Initialize() {
	Object3d::Initialize();
	// 見た目は持たないので描画しない
	SetIsDraw(false);
	Deactivate();
}

void EnemyHitbox::SetupAttachment(BaseRenderer* skinnedRenderer) {
	skinnedRenderer_ = skinnedRenderer;
	if (!skinnedRenderer_) {
		return;
	}
	// BoneAttachment の規約どおり、親は「オブジェクト本体」ではなく
	// スキンモデルを描いているレンダラーのトランスフォームにする
	GetWorldTransform()->SetParent(skinnedRenderer_->GetWorldTransform());
}

Vector3 EnemyHitbox::UndoParentScale(const Vector3& value) const {
	// OBBCollider はワールド行列から取り出したスケールを halfExtents に掛けるので、
	// ここで割っておかないとモデルの縮小率ぶんだけ判定が小さくなる
	const Vector3 parentScale = skinnedRenderer_
		? skinnedRenderer_->GetWorldTransform()->GetWorldScale()
		: Vector3{ 1.0f, 1.0f, 1.0f };
	auto undo = [](float v, float scale) {
		return (scale > 1e-4f) ? (v / scale) : v;
	};
	return { undo(value.x, parentScale.x), undo(value.y, parentScale.y), undo(value.z, parentScale.z) };
}

void EnemyHitbox::Activate(const std::string& jointName, const Vector3& halfExtents,
	const Vector3& offset, const DamageInfo& damage) {
	damage_ = damage;
	useBone_ = true;

	if (skinnedRenderer_) {
		attachment_.Initialize(skinnedRenderer_, jointName);
	}

	// ジョイント基準のオフセット。追従側のローカル座標がそのままオフセットになる
	GetWorldTransform()->GetTranslation() = UndoParentScale(offset);

	if (auto* collider = dynamic_cast<OBBCollider*>(GetCollider(name_))) {
		collider->GetColliderData().halfExtents = UndoParentScale(halfExtents);
	}

	active_ = true;
	SetColliderActive(true);
	// 有効化した時点で一度合わせておく（Apply を待つと1フレーム原点に出る）
	Apply();
}

void EnemyHitbox::ActivateOriented(const Vector3& halfExtents, const Vector3& offset,
	const DamageInfo& damage) {
	damage_ = damage;
	useBone_ = false;

	// ボーン追従をやめる。前の攻撃で入ったジョイント行列を消しておかないと、
	// 体の正面ではなくその骨の位置・向きに判定が出たままになる
	GetWorldTransform()->ClearAttachMatrix();
	GetWorldTransform()->GetRotation() = Identity();

	// 親はレンダラーなので、ローカル座標がそのまま体の正面基準になる（+Z がプレイヤー側）
	GetWorldTransform()->GetTranslation() = UndoParentScale(offset);

	if (auto* collider = dynamic_cast<OBBCollider*>(GetCollider(name_))) {
		collider->GetColliderData().halfExtents = UndoParentScale(halfExtents);
	}

	active_ = true;
	SetColliderActive(true);
}

void EnemyHitbox::Deactivate() {
	active_ = false;
	SetColliderActive(false);
}

void EnemyHitbox::Apply() {
	// 体の正面基準（ActivateOriented）の判定はレンダラーの子として付いていくだけなので、
	// ここで何もしなくてよい
	if (!active_ || !useBone_) {
		return;
	}
	// ジョイントが見つからないモデル（静的モデルに戻した等）ではそのままにしておく。
	// 敵の足元に判定が出るだけで、落ちはしない
	if (attachment_.IsValid()) {
		attachment_.Apply(GetWorldTransform());
	}
}

void EnemyHitbox::SetColliderActive(bool active) {
	// isActive は形状ごとのデータ（OBBData）側にあるのでキャストして触る
	if (auto* collider = dynamic_cast<OBBCollider*>(GetCollider(name_))) {
		collider->GetColliderData().isActive = active;
	}
}
