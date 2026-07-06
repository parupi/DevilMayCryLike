#include "ClearEvent.h"
#include <Scene/Transition/TransitionManager.h>
#include <Utility/DeltaTime.h>

ClearEvent::ClearEvent(std::string objectName) : BaseEvent(objectName, EventType::Clear) {
	Object3d::Initialize();
}

void ClearEvent::Update(float) {
	if (currentFrame_ < skipFrames_) {
		currentFrame_++;
	}

	if (currentFrame_ < skipFrames_) {
		return;
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

void ClearEvent::Execute() {
	isClear_ = true;

	CameraManager::GetInstance().SetActiveCamera("ClearCamera");
	TransitionManager::GetInstance().SetTransition("Fade");
}

void ClearEvent::AddTargetEnemy(Enemy* enemy) {
	targetEnemies_.push_back(enemy);
}
