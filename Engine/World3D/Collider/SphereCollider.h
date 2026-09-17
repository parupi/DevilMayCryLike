#pragma once
#include "BaseCollider.h"
#include "World3D/Object/Object3d.h"
#include <Math/Vector3.h>
#include "World3D/Camera/CameraManager.h"
#include "World3D/Collider/ColliderStructs.h"

class SphereCollider : public BaseCollider
{
private:
	SphereData sphereData_;

public:
	SphereCollider(std::string colliderName);
	void Initialize() override;
	void Update() override;
	void DrawDebug() override;

	CollisionShapeType GetShapeType() const override { return CollisionShapeType::Sphere; }

	Vector3 GetCenter() const;
	float GetRadius() const;

	SphereData& GetColliderData() { return sphereData_; }
	const SphereData& GetColliderData() const { return sphereData_; }

	void SetColliderActive(bool active) override { sphereData_.isActive = active; }
	bool IsColliderActive() const override { return sphereData_.isActive; }

private:
};
