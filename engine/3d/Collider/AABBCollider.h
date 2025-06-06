#pragma once
#include "BaseCollider.h"
#include "Object/Object3d.h"
#include <Vector3.h>
#include "Camera/CameraManager.h"

class AABBCollider : public BaseCollider
{
private:
	struct AABBData {
		Vector3 offsetMax = { 0.5f, 0.5f, 0.5f };
		Vector3 offsetMin = { -0.5f, -0.5f, -0.5f };
	}aabbData_;
public:
	AABBCollider(std::string colliderName);
	void Initialize() override;
	void Update() override;
	void DrawDebug() override;

	CollisionShapeType GetShapeType() const override { return CollisionShapeType::AABB; }

	const Vector3& GetMax();
	const Vector3& GetMin();

	AABBData& GetColliderData() { return aabbData_; }

private:
	Camera* camera_ = CameraManager::GetInstance()->GetActiveCamera().get();
};

