#pragma once
#include <math/Vector3.h>

class LockOnSystem;

class LockOnTarget
{
public:
	// ‰Šú‰»
	void Initialize(LockOnSystem* system);
	// I—¹
	void Finalize();

	Vector3 GetWorldPosition() const;
	bool IsLockable() const;

private:
	LockOnSystem* system_ = nullptr;
};

