#pragma once
#include "BaseCollider.h"
#include "World3D/Object/Object3d.h"
#include <Math/Vector3.h>
#include "World3D/Camera/CameraManager.h"
#include "ColliderStructs.h"

class AABBCollider : public BaseCollider
{
private:
	AABBData aabbData_;
public:
	AABBCollider(std::string colliderName);
	void Initialize() override;
	void Update() override;
	void DrawDebug() override;

	CollisionShapeType GetShapeType() const override { return CollisionShapeType::AABB; }

	const Vector3& GetMax() const { return max_; }
	const Vector3& GetMin() const { return min_; }
	const Vector3& GetSize() { return size_; }

	AABBData& GetColliderData() { return aabbData_; }
	const AABBData& GetColliderData() const { return aabbData_; }

	void SetColliderActive(bool active) override { aabbData_.isActive = active; }
	bool IsColliderActive() const override { return aabbData_.isActive; }

private:
	Vector3 max_;
	Vector3 min_;

	Vector3 size_;
};

