#pragma once
#include <Math/Vector3.h>

class LockOnSystem;
class Object3d;

class LockOnTarget
{
public:
	// 初期化
	void Initialize(LockOnSystem* system, Object3d* owner);
	// 終了
	void Finalize();

	const Vector3& GetWorldPosition() const;
	bool IsLockable() const;

private:
	LockOnSystem* system_ = nullptr;
	Object3d* owner_ = nullptr;
};

