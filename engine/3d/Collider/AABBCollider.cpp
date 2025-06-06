#include "AABBCollider.h"
#include <Primitive/PrimitiveDrawer.h>

AABBCollider::AABBCollider(std::string colliderName)
{
    name = colliderName;
    transform_ = std::make_unique<WorldTransform>();
    transform_->Initialize();
}

void AABBCollider::Initialize()
{

}

void AABBCollider::Update()
{
	if (owner_ && transform_->GetParent() == nullptr) {
		transform_->SetParent(owner_->GetWorldTransform());
	}

    transform_->TransferMatrix();
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

    PrimitiveDrawer* drawer = PrimitiveDrawer::GetInstance();
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

const Vector3& AABBCollider::GetMax()
{
    return transform_->GetWorldPos() + aabbData_.offsetMax;
}

const Vector3& AABBCollider::GetMin()
{
	return transform_->GetWorldPos() + aabbData_.offsetMin;
}
