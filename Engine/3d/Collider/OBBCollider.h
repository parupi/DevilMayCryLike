#pragma once
#include "BaseCollider.h"
#include "3d/Object/Object3d.h"
#include <math/Vector3.h>
#include "3d/Camera/CameraManager.h"
#include "ColliderStructs.h"

class OBBCollider : public BaseCollider
{
private:
	OBBData obbData_;

	Vector3 center_;
	Vector3 axes_[3];
	Vector3 worldHalfExtents_;

public:
	OBBCollider(std::string colliderName);
	void Initialize() override;
	void Update() override;
	void DrawDebug() override;

	CollisionShapeType GetShapeType() const override { return CollisionShapeType::OBB; }

	const Vector3& GetCenter() const { return center_; }
	const Vector3& GetAxis(int i) const { return axes_[i]; }
	const Vector3& GetWorldHalfExtents() const { return worldHalfExtents_; }

	OBBData& GetColliderData() { return obbData_; }

private:
	BaseCamera* camera_ = CameraManager::GetInstance()->GetActiveCamera();
};
