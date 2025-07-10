#pragma once
#include "BaseCollider.h"
#include "3d/Object/Object3d.h"
#include <math/Vector3.h>
#include "3d/Camera/CameraManager.h"
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
	// 法線から位置関係を取得
	static Vector3 CalculateCollisionNormal(AABBCollider* thisCollider, AABBCollider* otherCollider);

	CollisionShapeType GetShapeType() const override { return CollisionShapeType::AABB; }

	const Vector3& GetMax() { return max_; }
	const Vector3& GetMin() { return min_; }

	AABBData& GetColliderData() { return aabbData_; }

private:
	Camera* camera_ = CameraManager::GetInstance()->GetActiveCamera();

	Vector3 max_;
	Vector3 min_;
};

