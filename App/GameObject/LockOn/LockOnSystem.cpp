#include "LockOnSystem.h"
#include "Input/LockOnInput.h"
#include "World3D/Camera/CameraManager.h"
#include "GameObject/Character/Player/Player.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void LockOnSystem::Initialize(LockOnInput* input, Player* player) {
	input_ = input;
	player_ = player;

	reticle_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "reticle", "reticle.png");
	reticle_->SetSize({ 32.0f, 32.0f });
	reticle_->SetAnchorPoint({ 0.5f, 0.5f });
}

void LockOnSystem::Update() {
	// ロックオンが成立した瞬間を検知するため、更新前の状態を保持しておく
	bool hadTarget = currentTarget_ != nullptr;

	FindBestTarget();
	// ロックオン入力
	if (input_->PushLockOnKey()) {
		if (!currentTarget_) {
			// ロックオン先が無ければさがす
			currentTarget_ = FindBestTarget();
		}
	}
	else {
		// 入力が無ければロックオンを解除する
		currentTarget_ = nullptr;
	}

	// 無効チェック
	if (currentTarget_ && !currentTarget_->IsLockable()) {
		currentTarget_ = nullptr;
	}

	// ロックオンが成立した瞬間にチュートリアルを進める
	if (!hadTarget && currentTarget_) {
		player_->GetTutorialService()->StepTutorial(TutorialState::LockOn);
	}

	if (IsLockOn()) {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
	} else {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}

	if (IsLockOn()) {
		reticle_->SetPosition(CameraManager::GetInstance().GetCurrentCamera()->WorldToScreen(currentTarget_->GetWorldPosition(), 1280, 720));
		reticle_->Update();
	}
}

void LockOnSystem::RegisterTarget(LockOnTarget* target) {
	targets_.push_back(target);
}

void LockOnSystem::UnregisterTarget(LockOnTarget* target) {
	for (size_t i = 0; i < targets_.size(); ++i) {
		if (targets_[i] == target) {
			// currentTargetの処理
			if (currentTarget_ == target) {
				currentTarget_ = nullptr;
			}

			// swapしてpopする
			targets_[i] = targets_.back();
			targets_.pop_back();
			return;
		}
	}
}

LockOnTarget* LockOnSystem::FindBestTarget() {
	LockOnTarget* best = nullptr;
	float bestScore = FLT_MAX;

	for (auto* target : targets_) {
		if (!target->IsLockable()) continue;

		float score = CalculateScore(target);

		if (score < bestScore) {
			bestScore = score;
			best = target;
		}
	}

	return best;
}

float LockOnSystem::CalculateScore(LockOnTarget* target) {
	auto* camera = CameraManager::GetInstance().GetCurrentCamera();

	Vector3 camPos = camera->GetTranslate();
	Vector3 targetPos = target->GetWorldPosition();

	Vector3 toTarget = targetPos - camPos;
	toTarget.y = 0.0f;
	Vector3 dir = Normalize(toTarget);

	Vector3 forward = camera->GetForward();
	forward.y = 0.0f;
	forward = Normalize(forward);

	float dot = Dot(forward, dir);

	float distance = Length(toTarget);

	//PrimitiveLineDrawer::GetInstance().DrawWireSphere(targetPos, 1.0f, Vector4(1.0f, 0.0f, 0.0f, 1.0f));
	//Vector3 f = camera->GetForward();
	//ImGui::Begin("Debug");
	//ImGui::Text("Forward: %f %f %f\n", f.x, f.y, f.z);
	//ImGui::End();

	// ===== 背面除外 =====
	if (dot < 0.0f) {
		return FLT_MAX;
	}

	// ===== 視野角チェック =====
	float fov = camera->GetFovY();
	float cosHalfFov = cos(fov * 0.5f);

	if (dot < cosHalfFov) {
		return FLT_MAX;
	}

	// ===== 画面中心からのズレ =====
	float angle = acos(dot); // ラジアン

	// 正規化（0〜1）
	float screenDist = angle / (fov * 0.5f);

	// ===== スコア =====
	return distance * 0.3f + screenDist * 0.7f;
}