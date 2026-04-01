#include "LockOnSystem.h"
#include "input/LockOnInput.h"
#include <3d/Camera/CameraManager.h>
#include "GameObject/Character/Player/Player.h"

void LockOnSystem::Initialize(LockOnInput* input, Player* player)
{
	input_ = input;
	player_ = player;
}

void LockOnSystem::Update()
{
	// ロックオン入力
	if (input_->PushLockOnKey()) {
		if (!currentTarget_) {
			// ロックオン先が無ければさがす
			currentTarget_ = FindBestTarget();
		}
	} else {
		// 入力が無ければロックオンを解除する
		currentTarget_ = nullptr;
	}

	// 無効チェック
	if (currentTarget_ && !currentTarget_->IsLockable()) {
		currentTarget_ = nullptr;
	}
}

void LockOnSystem::RegisterTarget(LockOnTarget* target)
{
	targets_.push_back(target);
}

void LockOnSystem::UnregisterTarget(LockOnTarget* target)
{
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

LockOnTarget* LockOnSystem::FindBestTarget()
{
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

float LockOnSystem::CalculateScore(LockOnTarget* target)
{
	auto* camera = CameraManager::GetInstance()->GetCurrentCamera();

	Vector3 toTarget = target->GetWorldPosition() - player_->GetWorldTransform()->GetTranslation();
	// y軸は評価しない
	toTarget.y = 0.0f;
	float distance = Length(toTarget);

	Vector3 dir = Normalize(toTarget);
	float dot = Dot(camera->GetForward(), dir);

	// 背面は除外
	if (dot < 0.0f) {
		return FLT_MAX;
	}

	// 画面中央からの距離
	float screenDist = 1.0f - dot;

	// 重み付け
	return distance * 0.3f + screenDist * 0.7f;
}