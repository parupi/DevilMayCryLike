#include "ClearEvent.h"
#include <scene/SceneManager.h>
#include <scene/Transition/SceneTransitionController.h>
#include <scene/Transition/TransitionManager.h>

ClearEvent::ClearEvent(std::string objectName) : BaseEvent(objectName, EventType::Clear)
{
	Object3d::Initialize();
}

void ClearEvent::Update(float deltaTime)
{
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}

	if (currentFrame_ < skipFrames_) {
		return; // 最初の数フレームは処理しない
	}

	if (requested_) {
		waitTime_ += DeltaTime::GetDeltaTime();

		if (waitTime_ >= waitDuration_) {
			// 一度だけ実行
			requested_ = false;

			SceneTransitionController::GetInstance()->RequestSceneChange("CLEAR", true);
		}
	}

	if (isClear_) return;

	bool isTrigger = true;
	for (auto& enemy : targetEnemies_) {
		if (enemy && enemy->IsAlive()) {
			isTrigger = false;
		}
	}

	if (isTrigger) {
		Execute();
	}
}

void ClearEvent::Execute()
{
	isClear_ = true;

	CameraManager::GetInstance()->SetActiveCamera("ClearCamera");

	if (!player_) {
		player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
		if (!player_) return;
	}
	player_->Clear();

	// ここでは即遷移せず「遷移予約」だけする
	requested_ = true;
	waitTime_ = 0.0f;

	// 最初のフェード開始などは即実行するなら残してOK
	TransitionManager::GetInstance()->SetTransition("Fade");
}

void ClearEvent::AddTargetEnemy(Enemy* enemy)
{
	targetEnemies_.push_back(enemy);
}
