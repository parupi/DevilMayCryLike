#include "CollisionManager.h"
#include "Utility/Logger.h"
#include <cmath>


CollisionManager& CollisionManager::GetInstance()
{
	static CollisionManager instance;
	return instance;
}

void CollisionManager::Initialize()
{
}

void CollisionManager::Finalize()
{
}

void CollisionManager::Update()
{
    // 死んだオブジェクトを消す
    RemoveDeadObjects();

    // 生きているやつだけUpdate
    for (auto& collider : colliders_) {
        if (collider) {
            collider->Update();
        }
    }

    // 衝突判定してExit発行
    CheckAllCollisions();
}

void CollisionManager::DeleteAllCollider()
{
    for (auto& collider : colliders_) {
        collider->isAlive = false;
    }
}

void CollisionManager::Draw()
{
#ifdef _DEBUG
    for (auto& collider : colliders_) {
        if (collider) {
            collider->DrawDebug();
        }
    }
#endif // DEBUG
}

void CollisionManager::AddCollider(std::unique_ptr<BaseCollider> collider)
{
    colliders_.push_back(std::move(collider));
}

BaseCollider* CollisionManager::FindCollider(std::string colliderName)
{
    for (auto& collider : colliders_) {
        if (collider->name_ == colliderName) {
            return collider.get();
        }
    }
    Logger::Log("colliderが見つかりませんでした");
    return nullptr;
}

void CollisionManager::RemoveDeadObjects()
{
    std::vector<BaseCollider*> deadColliders;
    for (auto& obj : colliders_) {
        if (!obj->isAlive) {
            deadColliders.push_back(obj.get());
        }
    }

    for (auto it = previousCollisions_.begin(); it != previousCollisions_.end();) {
        BaseCollider* a = it->first;
        BaseCollider* b = it->second;

        bool shouldErase =
            std::find(deadColliders.begin(), deadColliders.end(), a) != deadColliders.end() ||
            std::find(deadColliders.begin(), deadColliders.end(), b) != deadColliders.end();

        if (shouldErase) {
            it = previousCollisions_.erase(it);
        } else {
            ++it;
        }
    }

    size_t before = colliders_.size();

    colliders_.erase(
        std::remove_if(colliders_.begin(), colliders_.end(),
            [](const std::unique_ptr<BaseCollider>& obj) {
                return !obj->isAlive;
            }),
        colliders_.end()
    );

    size_t after = colliders_.size();
    if (before != after) {
        char buf[128];
        sprintf_s(buf, "[CollisionManager] Removed %zu dead collider(s)\n", before - after);
        OutputDebugStringA(buf);
    }
}

const std::vector<BaseCollider*>& CollisionManager::GetCurrentHits(BaseCollider* collider) const
{
    static std::vector<BaseCollider*> results;
    results.clear();

    for (const auto& pair : currentCollisions_) {
        if (pair.first == collider) {
            results.push_back(pair.second);
        } else if (pair.second == collider) {
            results.push_back(pair.first);
        }
    }
    return results;
}

void CollisionManager::CheckAllCollisions()
{
    currentCollisions_.clear();

    size_t colliderCount = colliders_.size();
    for (size_t i = 0; i < colliderCount; ++i) {
        BaseCollider* a = colliders_[i].get();
        if (!a) continue;

        for (size_t j = i + 1; j < colliderCount; ++j) {
            BaseCollider* b = colliders_[j].get();
            if (!b) continue;

            if (CheckCollision(a, b)) {
                ColliderPair pair = std::minmax(a, b);
                 
                currentCollisions_.insert(pair);

                if (previousCollisions_.count(pair)) {
                    // すでに衝突していた
                    if (a->owner_) a->owner_->OnCollisionStay(b);
                    if (b->owner_) b->owner_->OnCollisionStay(a);
                } else {
                    // 今回初めて衝突
                    if (a->owner_) a->owner_->OnCollisionEnter(b);
                    if (b->owner_) b->owner_->OnCollisionEnter(a);
                }
            }
        }
    }

    // 衝突が終了したペアを検出
    for (const auto& pair : previousCollisions_) {
        if (!currentCollisions_.count(pair)) {
            if (pair.first->isAlive && pair.first->owner_) {
                pair.first->owner_->OnCollisionExit(pair.second);
            }
            if (pair.second->isAlive && pair.second->owner_) {
                pair.second->owner_->OnCollisionExit(pair.first);
            }
        }
    }

    // 今回の衝突情報を次回用に保存
    previousCollisions_ = currentCollisions_;
}

bool CollisionManager::CheckCollision(BaseCollider* a, BaseCollider* b)
{
    auto typeA = a->GetShapeType();
    auto typeB = b->GetShapeType();

    if (typeA == CollisionShapeType::Sphere && typeB == CollisionShapeType::Sphere) {
        return CheckSphereToSphereCollision(static_cast<SphereCollider*>(a), static_cast<SphereCollider*>(b));
    }
    if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::AABB) {
        return CheckAABBToAABBCollision(static_cast<AABBCollider*>(a), static_cast<AABBCollider*>(b));
    }
    if (typeA == CollisionShapeType::OBB && typeB == CollisionShapeType::OBB) {
        return CheckOBBToOBBCollision(static_cast<OBBCollider*>(a), static_cast<OBBCollider*>(b));
    }
    if (typeA == CollisionShapeType::OBB && typeB == CollisionShapeType::AABB) {
        return CheckOBBToAABBCollision(static_cast<OBBCollider*>(a), static_cast<AABBCollider*>(b));
    }
    if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::OBB) {
        return CheckOBBToAABBCollision(static_cast<OBBCollider*>(b), static_cast<AABBCollider*>(a));
    }
    return false;
}

bool CollisionManager::CheckAABBToAABBCollision(AABBCollider* a, AABBCollider* b)
{
    // コライダーがどちらもactiveになってるか確認
    if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return false;

    Vector3 MaxPosA = a->GetMax();
    Vector3 MaxPosB = b->GetMax();
    Vector3 MinPosA = a->GetMin();
    Vector3 MinPosB = b->GetMin();

    bool isTachX = MaxPosA.x > MinPosB.x && MinPosA.x < MaxPosB.x;
    bool isTachY = MaxPosA.y > MinPosB.y && MinPosA.y < MaxPosB.y;
    bool isTachZ =  MaxPosA.z >MinPosB.z && MinPosA.z < MaxPosB.z;

    return isTachX && isTachY && isTachZ;
}

bool CollisionManager::CheckSphereToSphereCollision(SphereCollider* a, SphereCollider* b)
{
    // コライダーがどちらもactiveになってるか確認
    if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return false;

    float distSq = Length(a->GetCenter() - b->GetCenter());
    float radiusSum = a->GetRadius() + b->GetRadius();
    return distSq <= (radiusSum * radiusSum);
}

bool CollisionManager::CheckOBBToOBBCollision(OBBCollider* a, OBBCollider* b)
{
    if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return false;

    const Vector3& heA = a->GetWorldHalfExtents();
    const Vector3& heB = b->GetWorldHalfExtents();

    // R[i][j] = Dot(A.axis[i], B.axis[j])
    // 分離軸定理(SAT)用の回転行列を構築
    float R[3][3], AbsR[3][3];
    const float eps = 1e-6f;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            R[i][j] = Dot(a->GetAxis(i), b->GetAxis(j));
            AbsR[i][j] = std::abs(R[i][j]) + eps;
        }
    }

    // Aの座標系での中心間ベクトル
    Vector3 diff = b->GetCenter() - a->GetCenter();
    float t[3] = {
        Dot(diff, a->GetAxis(0)),
        Dot(diff, a->GetAxis(1)),
        Dot(diff, a->GetAxis(2))
    };

    float heAv[3] = { heA.x, heA.y, heA.z };
    float heBv[3] = { heB.x, heB.y, heB.z };

    // Aの3軸でテスト
    for (int i = 0; i < 3; i++) {
        float ra = heAv[i];
        float rb = heBv[0]*AbsR[i][0] + heBv[1]*AbsR[i][1] + heBv[2]*AbsR[i][2];
        if (std::abs(t[i]) > ra + rb) return false;
    }

    // Bの3軸でテスト
    for (int j = 0; j < 3; j++) {
        float ra = heAv[0]*AbsR[0][j] + heAv[1]*AbsR[1][j] + heAv[2]*AbsR[2][j];
        float rb = heBv[j];
        float tProj = std::abs(t[0]*R[0][j] + t[1]*R[1][j] + t[2]*R[2][j]);
        if (tProj > ra + rb) return false;
    }

    // A.axis[i] x B.axis[j] の9軸でテスト
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
            int j1 = (j + 1) % 3, j2 = (j + 2) % 3;
            float ra = heAv[i1]*AbsR[i2][j] + heAv[i2]*AbsR[i1][j];
            float rb = heBv[j1]*AbsR[i][j2] + heBv[j2]*AbsR[i][j1];
            float tProj = std::abs(t[i2]*R[i1][j] - t[i1]*R[i2][j]);
            if (tProj > ra + rb) return false;
        }
    }

    return true;
}

bool CollisionManager::CheckOBBToAABBCollision(OBBCollider* obb, AABBCollider* aabb)
{
    if (!obb->GetColliderData().isActive || !aabb->GetColliderData().isActive) return false;

    // AABBの中心と半サイズ
    Vector3 aabbCenter = (aabb->GetMax() + aabb->GetMin()) * 0.5f;
    Vector3 aabbHalf   = (aabb->GetMax() - aabb->GetMin()) * 0.5f;
    const Vector3& obbHalf = obb->GetWorldHalfExtents();

    // R[i][j] = Dot(OBB.axis[i], AABB.axis[j])
    // AABBの軸はワールド軸なので R[i][j] = OBB.axis[i] の j番目の成分
    float R[3][3], AbsR[3][3];
    const float eps = 1e-6f;
    for (int i = 0; i < 3; i++) {
        const Vector3& ax = obb->GetAxis(i);
        float comp[3] = { ax.x, ax.y, ax.z };
        for (int j = 0; j < 3; j++) {
            R[i][j]    = comp[j];
            AbsR[i][j] = std::abs(comp[j]) + eps;
        }
    }

    // OBBローカル座標での中心間ベクトル
    Vector3 diff = aabbCenter - obb->GetCenter();
    float t[3] = {
        Dot(diff, obb->GetAxis(0)),
        Dot(diff, obb->GetAxis(1)),
        Dot(diff, obb->GetAxis(2))
    };
    float diffComp[3] = { diff.x, diff.y, diff.z };

    float obbHalfv[3]  = { obbHalf.x,  obbHalf.y,  obbHalf.z  };
    float aabbHalfv[3] = { aabbHalf.x, aabbHalf.y, aabbHalf.z };

    // OBBの3軸でテスト
    for (int i = 0; i < 3; i++) {
        float ra = obbHalfv[i];
        float rb = aabbHalfv[0]*AbsR[i][0] + aabbHalfv[1]*AbsR[i][1] + aabbHalfv[2]*AbsR[i][2];
        if (std::abs(t[i]) > ra + rb) return false;
    }

    // AABBの3軸 (ワールドX,Y,Z) でテスト
    for (int j = 0; j < 3; j++) {
        float ra = obbHalfv[0]*AbsR[0][j] + obbHalfv[1]*AbsR[1][j] + obbHalfv[2]*AbsR[2][j];
        float rb = aabbHalfv[j];
        if (std::abs(diffComp[j]) > ra + rb) return false;
    }

    // OBB.axis[i] x AABB.axis[j] の9軸でテスト
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int i1 = (i + 1) % 3, i2 = (i + 2) % 3;
            int j1 = (j + 1) % 3, j2 = (j + 2) % 3;
            float ra    = obbHalfv[i1]*AbsR[i2][j]  + obbHalfv[i2]*AbsR[i1][j];
            float rb    = aabbHalfv[j1]*AbsR[i][j2] + aabbHalfv[j2]*AbsR[i][j1];
            float tProj = std::abs(t[i2]*R[i1][j] - t[i1]*R[i2][j]);
            if (tProj > ra + rb) return false;
        }
    }

    return true;
}
