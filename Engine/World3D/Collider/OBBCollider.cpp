#include "OBBCollider.h"
#include <World3D/Primitive/PrimitiveLineDrawer.h>

OBBCollider::OBBCollider(std::string colliderName)
{
    name_ = colliderName;
    transform_ = std::make_unique<WorldTransform>();
    transform_->Initialize();
}

void OBBCollider::Initialize()
{
}

void OBBCollider::Update()
{
    if (owner_ && transform_->GetParent() == nullptr) {
        transform_->SetParent(owner_->GetWorldTransform());
    }

    transform_->TransferMatrix(CameraManager::GetInstance()->GetCurrentCamera());

    const Matrix4x4& mat = transform_->GetMatWorld();

    // ワールド行列の各行から各軸を抽出
    Vector3 row0 = { mat.m[0][0], mat.m[0][1], mat.m[0][2] };
    Vector3 row1 = { mat.m[1][0], mat.m[1][1], mat.m[1][2] };
    Vector3 row2 = { mat.m[2][0], mat.m[2][1], mat.m[2][2] };

    float scaleX = Length(row0);
    float scaleY = Length(row1);
    float scaleZ = Length(row2);

    axes_[0] = (scaleX > 1e-6f) ? row0 / scaleX : Vector3{ 1.0f, 0.0f, 0.0f };
    axes_[1] = (scaleY > 1e-6f) ? row1 / scaleY : Vector3{ 0.0f, 1.0f, 0.0f };
    axes_[2] = (scaleZ > 1e-6f) ? row2 / scaleZ : Vector3{ 0.0f, 0.0f, 1.0f };

    worldHalfExtents_ = {
        obbData_.halfExtents.x * scaleX,
        obbData_.halfExtents.y * scaleY,
        obbData_.halfExtents.z * scaleZ
    };

    // オフセットをローカル軸方向に適用
    Vector3 worldPos = transform_->GetWorldPos();
    center_ = worldPos
        + axes_[0] * obbData_.offset.x
        + axes_[1] * obbData_.offset.y
        + axes_[2] * obbData_.offset.z;
}

void OBBCollider::DrawDebug()
{
    const Vector3& c = center_;
    const Vector3& h = worldHalfExtents_;

    // OBBの8頂点を生成
    Vector3 corners[8] = {
        c - axes_[0]*h.x - axes_[1]*h.y - axes_[2]*h.z, // 0: ---
        c + axes_[0]*h.x - axes_[1]*h.y - axes_[2]*h.z, // 1: +--
        c + axes_[0]*h.x + axes_[1]*h.y - axes_[2]*h.z, // 2: ++-
        c - axes_[0]*h.x + axes_[1]*h.y - axes_[2]*h.z, // 3: -+-
        c - axes_[0]*h.x - axes_[1]*h.y + axes_[2]*h.z, // 4: --+
        c + axes_[0]*h.x - axes_[1]*h.y + axes_[2]*h.z, // 5: +-+
        c + axes_[0]*h.x + axes_[1]*h.y + axes_[2]*h.z, // 6: +++
        c - axes_[0]*h.x + axes_[1]*h.y + axes_[2]*h.z, // 7: -++
    };

    PrimitiveLineDrawer* drawer = PrimitiveLineDrawer::GetInstance();
    Vector4 color = { 1.0f, 0.5f, 0.0f, 1.0f }; // オレンジ

    // 底面 (z-)
    drawer->DrawLine(corners[0], corners[1], color);
    drawer->DrawLine(corners[1], corners[2], color);
    drawer->DrawLine(corners[2], corners[3], color);
    drawer->DrawLine(corners[3], corners[0], color);

    // 上面 (z+)
    drawer->DrawLine(corners[4], corners[5], color);
    drawer->DrawLine(corners[5], corners[6], color);
    drawer->DrawLine(corners[6], corners[7], color);
    drawer->DrawLine(corners[7], corners[4], color);

    // 側面の柱
    drawer->DrawLine(corners[0], corners[4], color);
    drawer->DrawLine(corners[1], corners[5], color);
    drawer->DrawLine(corners[2], corners[6], color);
    drawer->DrawLine(corners[3], corners[7], color);
}
