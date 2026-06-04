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

private:
	BaseCamera* camera_ = CameraManager::GetInstance().GetActiveCamera();
};
