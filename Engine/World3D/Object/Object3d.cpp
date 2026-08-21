#include "Object3d.h"
#include <algorithm>
#include "Object3dManager.h"
#include "Graphics/Resource/TextureManager.h"
#include <World3D/WorldTransform.h>
#include <numbers>
#include "Model/ModelManager.h"
#include "Model/Animation/SkinnedInstance.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI
#include <World3D/Camera/CameraManager.h>
#include <cmath>

namespace {
/// <summary>
/// 境界球がライトの視錐台（カスケード）に入っているか。
/// 平行光の直交投影を前提に、行ベクトル規約で clip 空間へ落として判定する。
/// </summary>
bool IsSphereInLightFrustum(const Matrix4x4& lightViewProj, const Vector3& center, float radius) {
	// clip = (center, 1) * lightViewProj
	float clip[4]{};
	for (int col = 0; col < 4; ++col) {
		clip[col] = center.x * lightViewProj.m[0][col]
			+ center.y * lightViewProj.m[1][col]
			+ center.z * lightViewProj.m[2][col]
			+ lightViewProj.m[3][col];
	}

	const float w = clip[3];
	// 想定外の投影（透視など）ならカリングしない
	if (w <= 1e-6f) return true;

	// 半径をclip空間へ。出力成分ごとのスケールは、その列ベクトルの長さになる
	for (int col = 0; col < 3; ++col) {
		const float sx = lightViewProj.m[0][col];
		const float sy = lightViewProj.m[1][col];
		const float sz = lightViewProj.m[2][col];
		const float scaledRadius = radius * std::sqrt(sx * sx + sy * sy + sz * sz);

		if (clip[col] - scaledRadius > w) return false;

		// 手前側(z<0)は弾かない。ライトとニアクリップの間にある物も影は落とすため。
		// x,y は [-w, w] の外に出たら見えない
		if (col != 2 && clip[col] + scaledRadius < -w) return false;
	}
	return true;
}
} // namespace

Object3d::Object3d(std::string objectName) {
	name_ = objectName;
	isAlive = true;
	Initialize();
}

void Object3d::Initialize() {
	objectManager_ = &Object3dManager::GetInstance();

	// 派生クラスのコンストラクタからも呼ばれるため、二度目はトランスフォームを作り直さない。
	// 作り直すと、生成後に設定した位置が消える（ステージデータからの生成順がこれ）うえ、
	// 定数バッファも作り直しになる
	if (!transform_) {
		transform_ = std::make_unique<WorldTransform>();
		transform_->Initialize();
	}

	camera_ = objectManager_->GetDefaultCamera();
}

void Object3d::Update(float) {
	UpdateTransformOnly();

	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->Update(transform_.get());
	}
}

void Object3d::UpdateTransformOnly() {
	// CameraManager から引くと名前でのmap検索になるので、
	// CameraManager::Update() が毎フレーム流し込んでいる現在のカメラを使う
	camera_ = Object3dManager::GetInstance().GetDefaultCamera();
	if (!camera_) {
		camera_ = CameraManager::GetInstance().GetCurrentCamera();
	}

	// TransferMatrix は材料が前フレームと同じなら中で丸ごと省くので、
	// 動かないオブジェクトを毎フレーム通しても負荷にならない
	transform_->TransferMatrix(camera_);
}

void Object3d::DispatchSkinning() {
	if (!isDraw) return;
	for (size_t i = 0; i < renders_.size(); i++) {
		if (auto* instance = renders_[i]->GetSkinnedInstance()) {
			instance->DispatchSkinning();
		}
	}
}

void Object3d::Draw() {
	// 非表示設定なら描画しない
	if (!isDraw) return;
	switch (drawOption_.drawPath) {
	case DrawPath::Forward:
		for (size_t i = 0; i < renders_.size(); i++) {
			Object3dManager::GetInstance().GetDxManager()->GetCommandList()->SetPipelineState(Object3dManager::GetInstance().GetPsoManager()->GetObjectPSO(BlendMode::kNormal));
			Object3dManager::GetInstance().GetDxManager()->GetCommandList()->SetGraphicsRootSignature(Object3dManager::GetInstance().GetPsoManager()->GetObjectSignature());
			Object3dManager::GetInstance().GetDxManager()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			renders_[i]->Draw();
		}
		break;
	case DrawPath::Deferred:
		for (auto* d : deferredDrawables_) {
			d->DrawGBuffer();
		}
		break;
	}
}

void Object3d::DrawShadow(const Matrix4x4& lightViewProj) {
	// 非表示のオブジェクトは影も落とさない
	if (!isDraw) return;
	if (drawOption_.drawPath != DrawPath::Deferred) return;
	for (auto* s : shadowCasters_) {
		// このカスケードに入らないものは描かない。
		// カスケードごとに全オブジェクトを流すと、ステージが増えるほど描画コマンドが無駄に増える
		Vector3 center{};
		float radius = 0.0f;
		if (s->GetShadowBoundingSphere(center, radius) && !IsSphereInLightFrustum(lightViewProj, center, radius)) {
			continue;
		}
		s->DrawShadow();
	}
}

void Object3d::ResetObject() {
	for (auto& collider : colliders_) {
		collider->isAlive = false;
	}
	colliders_.clear();

	for (auto& renderer : renders_) {
		renderer->isAlive = false;
	}
	renders_.clear();
	deferredDrawables_.clear();
	shadowCasters_.clear();
}

#ifdef _DEBUG
void Object3d::DebugGui() {
	if (ImGui::TreeNode("Transform")) {
		transform_->DebugGui();
		ImGui::TreePop();
	}

	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->DebugGui(i);
	}
}
#endif // _DEBUG

void Object3d::OnCollisionEnter(BaseCollider* other) {
	other;
}

void Object3d::OnCollisionStay(BaseCollider* other) {
	other;
}

void Object3d::OnCollisionExit(BaseCollider* other) {
	other;
}

void Object3d::AddRenderer(BaseRenderer* renderer) {
	renders_.push_back(renderer);
	if (auto* d = dynamic_cast<IDeferredDrawable*>(renderer)) {
		deferredDrawables_.push_back(d);
	}
	if (auto* s = dynamic_cast<IShadowCaster*>(renderer)) {
		shadowCasters_.push_back(s);
	}
}

void Object3d::AddCollider(BaseCollider* collider) {
	collider->SetOwner(this);
	colliders_.push_back(collider);
}

void Object3d::RemoveCollider(BaseCollider* collider) {
	if (!collider) {
		return;
	}
	// 実体は CollisionManager が持っているので、生存フラグを落として自分の参照だけ外す。
	// 参照を残したままだと RemoveDeadObjects() で解放された後にぶら下がる
	collider->isAlive = false;
	collider->owner_ = nullptr;
	colliders_.erase(std::remove(colliders_.begin(), colliders_.end(), collider), colliders_.end());
}

BaseRenderer* Object3d::GetRenderer(std::string name) {
	for (auto& render : renders_) {
		if (render->name_ == name) {
			return render;
		}
	}
	Logger::Log("renderが見つかりませんでした");
	return nullptr;
}

BaseCollider* Object3d::GetCollider(std::string name) {
	for (auto& collider : colliders_) {
		if (collider->name_ == name) {
			return collider;
		}
	}
	Logger::Log("colliderが見つかりませんでした");
	return nullptr;
}
