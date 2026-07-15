#include "LockOnSystem.h"
#include "Input/LockOnInput.h"
#include "World3D/Camera/CameraManager.h"
#include "GameObject/Character/Player/Player.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void LockOnSystem::Initialize(LockOnInput* input, Player* player) {
	input_ = input;
	player_ = player;

	reticle_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "reticle", "reticle.png");
	reticle_->SetSize({ 32.0f, 32.0f });
	reticle_->SetAnchorPoint({ 0.5f, 0.5f });
}

void LockOnSystem::Update() {
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

	// 無効チェック：対象が消滅した、または画面外に出た場合は解除する
	if (currentTarget_ && (!currentTarget_->IsLockable() || !IsTargetVisible(currentTarget_))) {
		currentTarget_ = nullptr;
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

bool LockOnSystem::IsTargetVisible(LockOnTarget* target) const {
	auto* camera = CameraManager::GetInstance().GetCurrentCamera();
	return camera->IsInView(target->GetWorldPosition());
}

float LockOnSystem::CalculateScore(LockOnTarget* target) {
	auto* camera = CameraManager::GetInstance().GetCurrentCamera();

	Vector3 targetPos = target->GetWorldPosition();

	// ===== 視野内チェック（前方かつ画面内、水平・垂直とも判定する） =====
	Vector3 ndcPos;
	if (!camera->IsInView(targetPos, &ndcPos)) {
		return FLT_MAX;
	}

	float distance = Length(targetPos - camera->GetTranslate());

	// ===== 画面中心からのズレ（NDC原点からの距離。0〜1程度） =====
	float screenDist = Length(Vector3(ndcPos.x, ndcPos.y, 0.0f));

	// ===== スコア =====
	return distance * 0.3f + screenDist * 0.7f;
}