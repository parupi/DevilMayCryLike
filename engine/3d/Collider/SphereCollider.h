#pragma once
#include "BaseCollider.h"
#include "Object/Object3d.h"
#include <Vector3.h>
#include "Camera/CameraManager.h"

class SphereCollider : public BaseCollider
{
private:
	struct SphereData {
		float radius = 0.5f;
		Vector3 offset = { 0.0f, 0.0f, 0.0f };
	} sphereData_;

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
	Camera* camera_ = CameraManager::GetInstance()->GetActiveCamera().get();
};
