#include "LockOnTarget.h"
#include "LockOnSystem.h"

void LockOnTarget::Initialize(LockOnSystem* system)
{
	system_ = system;
	system_->RegisterTarget(this);
}

void LockOnTarget::Finalize()
{
	system_->UnregisterTarget(this);
}

Vector3 LockOnTarget::GetWorldPosition() const
{
	return Vector3();
}

bool LockOnTarget::IsLockable() const
{
	return false;
}
