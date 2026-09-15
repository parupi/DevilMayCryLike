#include "ClearEvent.h"
#include <Scene/Transition/TransitionManager.h>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Object/Object3dManager.h>

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
	for (const auto& enemyName : targetEnemyNames_) {
		// 削除済み（FindObjectで見つからない）＝撃破済み
		auto* enemy = dynamic_cast<Enemy*>(Object3dManager::GetInstance().FindObject(enemyName));
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
	if (!enemy) return;
	targetEnemyNames_.push_back(enemy->name_);
}
