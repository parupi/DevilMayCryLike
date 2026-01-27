#pragma once
#include "BaseCollider.h"
#include "3d/Object/Object3d.h"
#include <math/Vector3.h>
#include "3d/Camera/CameraManager.h"
#include "3d/Collider/ColliderStructs.h"

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
	BaseCamera* camera_ = CameraManager::GetInstance()->GetActiveCamera();
};
