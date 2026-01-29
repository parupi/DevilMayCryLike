#pragma once
#include <3d/Collider/AABBCollider.h>

struct CollisionHit
{
    BaseCollider* other;
    Vector3 normal;
    float penetration;
};

class PlayerCollider
{
public:
    void Initialize(BaseCollider* collider);

    // CollisionManager から呼ばれる
    void OnCollisionEnter(BaseCollider* other);
    void OnCollisionStay(BaseCollider* other);
    void OnCollisionExit(BaseCollider* other);

    // 毎フレーム呼ぶ
    void Update();

    // Resolver 用
    const std::vector<CollisionHit>& GetHits() const { return hits_; }

    bool IsOnGround() const { return isOnGround_; }

private:
    BaseCollider* collider_ = nullptr;

    // 今フレーム当たっている Collider
    std::vector<BaseCollider*> currentColliders_;

    // Resolver に渡す加工済み情報
    std::vector<CollisionHit> hits_;

    bool isOnGround_ = false;

private:
    bool ComputeHit(
        BaseCollider* self,
        BaseCollider* other,
        CollisionHit& outHit
    );

    bool ComputeAABBvsAABB(
        AABBCollider* a,
        AABBCollider* b,
        CollisionHit& outHit
    );
};

