#include "LockOnSystem.h"
#include "Input/LockOnInput.h"
#include "World3D/Camera/CameraManager.h"
#include "GameObject/Character/Player/Player.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "Graphics/Resource/TextureManager.h"
#include <algorithm>
#include <cmath>
#include <vector>

void LockOnSystem::Initialize(LockOnInput* input, Player* player) {
	input_ = input;
	player_ = player;

	reticle_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "reticle", "reticle.png");
	reticle_->SetSize({ 32.0f, 32.0f });
	reticle_->SetAnchorPoint({ 0.5f, 0.5f });

	// HPリング用のテクスチャをプロシージャル生成する（白いドーナツ型・縁は1pxで滑らか）
	constexpr uint32_t kRingTexSize = 64;
	constexpr float kOuterRadius = 30.0f; // 外周半径[px]
	constexpr float kInnerRadius = 24.0f; // 内周半径[px]（差がリングの太さ）
	std::vector<uint8_t> ringPixels(kRingTexSize * kRingTexSize * 4);
	const float center = (kRingTexSize - 1) * 0.5f;
	for (uint32_t y = 0; y < kRingTexSize; ++y) {
		for (uint32_t x = 0; x < kRingTexSize; ++x) {
			float dx = static_cast<float>(x) - center;
			float dy = static_cast<float>(y) - center;
			float dist = std::sqrt(dx * dx + dy * dy);
			float alpha = std::clamp(kOuterRadius - dist, 0.0f, 1.0f) * std::clamp(dist - kInnerRadius, 0.0f, 1.0f);
			size_t index = (static_cast<size_t>(y) * kRingTexSize + x) * 4;
			ringPixels[index + 0] = 255;
			ringPixels[index + 1] = 255;
			ringPixels[index + 2] = 255;
			ringPixels[index + 3] = static_cast<uint8_t>(alpha * 255.0f);
		}
	}
	TextureManager::GetInstance().LoadTextureFromMemory("__LOCKON_HP_RING__.png", ringPixels.data(), kRingTexSize, kRingTexSize);

	// レティクルより一回り大きいリングとして表示する
	hpRing_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "lockOnHpRing", "__LOCKON_HP_RING__.png");
	hpRing_->SetSize({ 48.0f, 48.0f });
	hpRing_->SetAnchorPoint({ 0.5f, 0.5f });
	hpRing_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
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

	// 無効チェック：対象が消滅した、または画面外に出た場合は解除する
	if (currentTarget_ && (!currentTarget_->IsLockable() || !IsTargetVisible(currentTarget_))) {
		currentTarget_ = nullptr;
	}

	// ロックオンが成立した瞬間にチュートリアルを進める
	if (!hadTarget && currentTarget_) {
		player_->GetTutorialService()->StepTutorial(TutorialState::LockOn);
	}

	if (IsLockOn()) {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		hpRing_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
		// 敵の残りHP割合に応じて、リングが上から時計回りに欠けていく（DMC風のHP表示）
		hpRing_->SetRadialFill(currentTarget_->GetHpRatio());
	} else {
		reticle_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
		hpRing_->SetColor({ 1.0f, 1.0f, 1.0f, 0.0f });
	}

	if (IsLockOn()) {
		auto screenPos = CameraManager::GetInstance().GetCurrentCamera()->WorldToScreen(currentTarget_->GetWorldPosition(), 1280, 720);
		reticle_->SetPosition(screenPos);
		reticle_->Update();
		hpRing_->SetPosition(screenPos);
		hpRing_->Update();
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