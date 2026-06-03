#include "LockOnTarget.h"
#include "LockOnSystem.h"
#include "World3D/Object/Object3d.h"

void LockOnTarget::Initialize(LockOnSystem* system, Object3d* owner)
{
	system_ = system;
	system_->RegisterTarget(this);

	owner_ = owner;
}

void LockOnTarget::Finalize()
{
	if (system_) {
		system_->UnregisterTarget(this);
	}
}

Vector3 LockOnTarget::GetWorldPosition() const
{
	return owner_->GetWorldTransform()->GetTranslation();
}

bool LockOnTarget::IsLockable() const
{
	return true;
}
