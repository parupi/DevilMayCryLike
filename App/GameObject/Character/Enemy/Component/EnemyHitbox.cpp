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

void EnemyHitbox::Activate(const std::string& jointName, const Vector3& halfExtents,
	const Vector3& offset, const DamageInfo& damage) {
	damage_ = damage;

	if (skinnedRenderer_) {
		attachment_.Initialize(skinnedRenderer_, jointName);
	}

	// 親（レンダラー）の縮小を打ち消して、引数をワールド単位として扱えるようにする。
	// OBBCollider はワールド行列から取り出したスケールを halfExtents に掛けるので、
	// ここで割っておかないとモデルの縮小率ぶんだけ判定が小さくなる
	const Vector3 parentScale = skinnedRenderer_
		? skinnedRenderer_->GetWorldTransform()->GetWorldScale()
		: Vector3{ 1.0f, 1.0f, 1.0f };
	auto undoScale = [](float value, float scale) {
		return (scale > 1e-4f) ? (value / scale) : value;
	};

	// ジョイント基準のオフセット。追従側のローカル座標がそのままオフセットになる
	GetWorldTransform()->GetTranslation() = {
		undoScale(offset.x, parentScale.x),
		undoScale(offset.y, parentScale.y),
		undoScale(offset.z, parentScale.z)
	};

	if (auto* collider = dynamic_cast<OBBCollider*>(GetCollider(name_))) {
		collider->GetColliderData().halfExtents = {
			undoScale(halfExtents.x, parentScale.x),
			undoScale(halfExtents.y, parentScale.y),
			undoScale(halfExtents.z, parentScale.z)
		};
	}

	active_ = true;
	SetColliderActive(true);
	// 有効化した時点で一度合わせておく（Apply を待つと1フレーム原点に出る）
	Apply();
}

void EnemyHitbox::Deactivate() {
	active_ = false;
	SetColliderActive(false);
}

void EnemyHitbox::Apply() {
	if (!active_) {
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
