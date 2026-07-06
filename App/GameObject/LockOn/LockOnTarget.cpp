#include "LockOnTarget.h"
#include "LockOnSystem.h"
#include "World3D/Object/Object3d.h"
#include "GameObject/Character/Enemy/Enemy.h"

void LockOnTarget::Initialize(LockOnSystem* system, Object3d* owner) {
	system_ = system;
	system_->RegisterTarget(this);

	owner_ = owner;
}

void LockOnTarget::Finalize() {
	if (system_) {
		system_->UnregisterTarget(this);
	}
}

Vector3 LockOnTarget::GetWorldPosition() const {
	return owner_->GetWorldTransform()->GetTranslation();
}

bool LockOnTarget::IsLockable() const {
	bool flag = true;
	// 敵が死亡している場合はロックオン対象から除外する
	if (auto* enemy = dynamic_cast<Enemy*>(owner_)) {
		if (!enemy->IsActive()) {
			flag = false;
		} else if (!enemy->IsAlive()) {
			flag = false;
		}
	}
	return flag;
}
