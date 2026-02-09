#include "AABBCollider.h"
#include <3d/Primitive/PrimitiveLineDrawer.h>

AABBCollider::AABBCollider(std::string colliderName)
{
    name_ = colliderName;
    transform_ = std::make_unique<WorldTransform>();
    transform_->Initialize();
}

void AABBCollider::Initialize()
{

}

void AABBCollider::Update()
{
    max_ = transform_->GetWorldPos() + aabbData_.offsetMax;
    min_ = transform_->GetWorldPos() + aabbData_.offsetMin;

    size_ = max_ - min_;

	if (owner_ && transform_->GetParent() == nullptr) {
		transform_->SetParent(owner_->GetWorldTransform());
	}
  
    transform_->TransferMatrix(CameraManager::GetInstance()->GetCurrentCamera());
}

void AABBCollider::DrawDebug()
{
    Vector3 min = GetMin();
    Vector3 max = GetMax();

    // 8頂点
    Vector3 p[8] = {
        {min.x, min.y, min.z}, // 0
        {max.x, min.y, min.z}, // 1
        {max.x, max.y, min.z}, // 2
        {min.x, max.y, min.z}, // 3
        {min.x, min.y, max.z}, // 4
        {max.x, min.y, max.z}, // 5
        {max.x, max.y, max.z}, // 6
        {min.x, max.y, max.z}  // 7
    };

    PrimitiveLineDrawer* drawer = PrimitiveLineDrawer::GetInstance();
    Vector4 color = { 0.0f, 1.0f, 0.0f, 1.0f }; // 黄色

    // 底面
    drawer->DrawLine(p[0], p[1], color);
    drawer->DrawLine(p[1], p[2], color);
    drawer->DrawLine(p[2], p[3], color);
    drawer->DrawLine(p[3], p[0], color);

    // 上面
    drawer->DrawLine(p[4], p[5], color);
    drawer->DrawLine(p[5], p[6], color);
    drawer->DrawLine(p[6], p[7], color);
    drawer->DrawLine(p[7], p[4], color);

    // 側面の柱
    drawer->DrawLine(p[0], p[4], color);
    drawer->DrawLine(p[1], p[5], color);
    drawer->DrawLine(p[2], p[6], color);
    drawer->DrawLine(p[3], p[7], color);
}

Vector3 AABBCollider::CalculateCollisionNormal(AABBCollider* thisCollider, AABBCollider* otherCollider)
{
    Vector3 outNormal{};

    // 中心と半サイズ
    Vector3 centerA = (thisCollider->GetMin() + thisCollider->GetMax()) * 0.5f;
    Vector3 centerB = (otherCollider->GetMin() + otherCollider->GetMax()) * 0.5f;

    Vector3 halfA = (thisCollider->GetMax() - thisCollider->GetMin()) * 0.5f;
    Vector3 halfB = (otherCollider->GetMax() - otherCollider->GetMin()) * 0.5f;

    Vector3 delta = centerA - centerB;
    Vector3 absDelta = Vector3(std::abs(delta.x), std::abs(delta.y), std::abs(delta.z));
    Vector3 overlap = (halfA + halfB) - absDelta;

    // 衝突していない場合はゼロベクトルを返す（念のため）
    if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) {
        return Vector3(0, 0, 0);
    }

    // 最小めり込み軸を判断
    if (overlap.x < overlap.y && overlap.x < overlap.z) {
        outNormal = { (delta.x < 0) ? -1.0f : 1.0f, 0, 0 };
    } else if (overlap.y < overlap.z) {
        outNormal = { 0, (delta.y < 0) ? -1.0f : 1.0f, 0 };
    } else {
        outNormal = { 0, 0, (delta.z < 0) ? -1.0f : 1.0f };
    }

    return outNormal;
}
