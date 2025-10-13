#include "ClearEvent.h"
#include <scene/SceneManager.h>
#include <scene/Transition/SceneTransitionController.h>

ClearEvent::ClearEvent(std::string objectName) : BaseEvent(objectName, EventType::Clear)
{
	Object3d::Initialize();
}

void ClearEvent::Update()
{
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}

	if (currentFrame_ < skipFrames_) {
		return; // 最初の数フレームは処理しない
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

	SceneTransitionController::GetInstance()->RequestSceneChange("TITLE", true);
}

void ClearEvent::AddTargetEnemy(Enemy* enemy)
{
	targetEnemies_.push_back(enemy);
}
