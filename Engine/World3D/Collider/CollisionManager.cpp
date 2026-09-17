#include "CollisionManager.h"
#include "Utility/Logger.h"
#include "Utility/ScopeProfiler.h"
#include "Math/MathUtils.h"
#include <cmath>
#include <limits>
#ifdef _DEBUG
#include "Editor/Core/EditorDebugDraw.h"
#endif


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
    {
        PROF_SCOPE("Col:RemoveDead");
        // 死んだオブジェクトを消す
        RemoveDeadObjects();
    }

    {
        PROF_SCOPE("Col:各Update");
        // 生きているやつだけUpdate
        for (auto& collider : colliders_) {
            if (collider) {
                collider->Update();
            }
        }
    }

    {
        PROF_SCOPE("Col:総当たり判定");
        // 衝突判定してExit発行
        CheckAllCollisions();
    }
    PROF_COUNT("Col:コライダー数", colliders_.size());
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
    // 表示のON/OFFはエディタのDebug Drawメニューに集約している
    if (!EditorDebugDraw::IsEnabled(EditorDebugDraw::Flag::Collider)) {
        return;
    }
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

    // 死んだコライダーが無ければ、衝突ペアの走査ごと省ける。
    // この関数はフレームに2回（RemoveObjects と Update）呼ばれるので効く
    if (deadColliders.empty()) return;

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

    const size_t colliderCount = colliders_.size();

    // ブロードフェーズ用に、各コライダーのワールドAABBを1フレーム分だけ作る。
    // 総当たりの前にこれで弾いておくと、遠いペアで15軸のSATを回さずに済む
    broadPhaseBounds_.resize(colliderCount);
    for (size_t i = 0; i < colliderCount; ++i) {
        CalcWorldBounds(colliders_[i].get(), broadPhaseBounds_[i]);
    }

    size_t pairCount = 0;
    for (size_t i = 0; i < colliderCount; ++i) {
        BaseCollider* a = colliders_[i].get();
        if (!a) continue;

        for (size_t j = i + 1; j < colliderCount; ++j) {
            BaseCollider* b = colliders_[j].get();
            if (!b) continue;

            // 動かない地形同士は判定しない。
            // ステージのブロックが数十個あると、総ペアの大半がこれで占められる
            if (a->category_ == CollisionCategory::Ground && b->category_ == CollisionCategory::Ground) continue;

            // ワールドAABBが重なっていなければ詳細判定は不要
            if (!OverlapBounds(broadPhaseBounds_[i], broadPhaseBounds_[j])) continue;

            ++pairCount;
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

    PROF_COUNT("Col:判定ペア数", pairCount);
    PROF_COUNT("Col:接触ペア数", currentCollisions_.size());
}

void CollisionManager::CalcWorldBounds(const BaseCollider* collider, ColliderBounds& out)
{
    // 判定対象外にしたいので、取れなかったものは「どことも重ならないAABB」にしておく
    constexpr float kInf = (std::numeric_limits<float>::max)();
    out.min = { kInf, kInf, kInf };
    out.max = { -kInf, -kInf, -kInf };
    if (!collider) return;

    switch (collider->GetShapeType()) {
    case CollisionShapeType::AABB: {
        auto* aabb = static_cast<const AABBCollider*>(collider);
        out.min = aabb->GetMin();
        out.max = aabb->GetMax();
        break;
    }
    case CollisionShapeType::OBB: {
        auto* obb = static_cast<const OBBCollider*>(collider);
        const Vector3& half = obb->GetWorldHalfExtents();
        // 各軸を半径ぶん伸ばした絶対値の和が、OBBを包むAABBの半サイズになる
        Vector3 extent{};
        for (int axis = 0; axis < 3; ++axis) {
            const Vector3& dir = obb->GetAxis(axis);
            const float h = (axis == 0) ? half.x : (axis == 1) ? half.y : half.z;
            extent.x += std::abs(dir.x) * h;
            extent.y += std::abs(dir.y) * h;
            extent.z += std::abs(dir.z) * h;
        }
        const Vector3& center = obb->GetCenter();
        out.min = center - extent;
        out.max = center + extent;
        break;
    }
    case CollisionShapeType::Sphere: {
        auto* sphere = static_cast<const SphereCollider*>(collider);
        const float r = sphere->GetRadius();
        const Vector3& center = sphere->GetCenter();
        out.min = center - Vector3{ r, r, r };
        out.max = center + Vector3{ r, r, r };
        break;
    }
    }
}

bool CollisionManager::OverlapBounds(const ColliderBounds& a, const ColliderBounds& b)
{
    return a.max.x >= b.min.x && a.min.x <= b.max.x
        && a.max.y >= b.min.y && a.min.y <= b.max.y
        && a.max.z >= b.min.z && a.min.z <= b.max.z;
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

    float dist = Length(a->GetCenter() - b->GetCenter());
    float radiusSum = a->GetRadius() + b->GetRadius();
    return dist <= radiusSum;
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

PenetrationResult CollisionManager::CalculatePenetration(BaseCollider* mover, BaseCollider* blocker,
    PenetrationAxis axisMode)
{
    if (!mover || !blocker) return PenetrationResult{};

    static const Vector3 kIdentityAxes[3] = {
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
    };

    const CollisionShapeType typeA = mover->GetShapeType();
    const CollisionShapeType typeB = blocker->GetShapeType();

    if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::AABB) {
        auto* a = static_cast<AABBCollider*>(mover);
        auto* b = static_cast<AABBCollider*>(blocker);
        if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return PenetrationResult{};

        Vector3 centerA = (a->GetMin() + a->GetMax()) * 0.5f;
        Vector3 halfA = (a->GetMax() - a->GetMin()) * 0.5f;
        Vector3 centerB = (b->GetMin() + b->GetMax()) * 0.5f;
        Vector3 halfB = (b->GetMax() - b->GetMin()) * 0.5f;

        return CalculateBoxPenetration(centerA, kIdentityAxes, halfA, centerB, kIdentityAxes, halfB, axisMode);
    }

    if (typeA == CollisionShapeType::OBB && typeB == CollisionShapeType::OBB) {
        auto* a = static_cast<OBBCollider*>(mover);
        auto* b = static_cast<OBBCollider*>(blocker);
        if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return PenetrationResult{};

        Vector3 axesA[3] = { a->GetAxis(0), a->GetAxis(1), a->GetAxis(2) };
        Vector3 axesB[3] = { b->GetAxis(0), b->GetAxis(1), b->GetAxis(2) };

        return CalculateBoxPenetration(a->GetCenter(), axesA, a->GetWorldHalfExtents(), b->GetCenter(), axesB, b->GetWorldHalfExtents(), axisMode);
    }

    if (typeA == CollisionShapeType::OBB && typeB == CollisionShapeType::AABB) {
        auto* a = static_cast<OBBCollider*>(mover);
        auto* b = static_cast<AABBCollider*>(blocker);
        if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return PenetrationResult{};

        Vector3 axesA[3] = { a->GetAxis(0), a->GetAxis(1), a->GetAxis(2) };
        Vector3 centerB = (b->GetMin() + b->GetMax()) * 0.5f;
        Vector3 halfB = (b->GetMax() - b->GetMin()) * 0.5f;

        return CalculateBoxPenetration(a->GetCenter(), axesA, a->GetWorldHalfExtents(), centerB, kIdentityAxes, halfB, axisMode);
    }

    if (typeA == CollisionShapeType::AABB && typeB == CollisionShapeType::OBB) {
        auto* a = static_cast<AABBCollider*>(mover);
        auto* b = static_cast<OBBCollider*>(blocker);
        if (!a->GetColliderData().isActive || !b->GetColliderData().isActive) return PenetrationResult{};

        Vector3 centerA = (a->GetMin() + a->GetMax()) * 0.5f;
        Vector3 halfA = (a->GetMax() - a->GetMin()) * 0.5f;
        Vector3 axesB[3] = { b->GetAxis(0), b->GetAxis(1), b->GetAxis(2) };

        return CalculateBoxPenetration(centerA, kIdentityAxes, halfA, b->GetCenter(), axesB, b->GetWorldHalfExtents(), axisMode);
    }

    return PenetrationResult{};
}

PenetrationResult CollisionManager::CalculateBoxPenetration(
    const Vector3& centerA, const Vector3 axesA[3], const Vector3& halfA,
    const Vector3& centerB, const Vector3 axesB[3], const Vector3& halfB,
    PenetrationAxis axisMode)
{
    // 「水平な軸」とみなす傾きのしきい値。ヨー回転しかしないキャラの箱なら
    // 水平軸のY成分は0なので、浮動小数の誤差を吸収できればよい
    constexpr float kHorizontalAxisTolerance = 0.05f;

    // 分離軸の候補: Aの3軸 + Bの3軸 + それぞれのクロス積9本（最大15本）
    Vector3 candidateAxes[15];
    int axisCount = 0;

    for (int i = 0; i < 3; ++i) candidateAxes[axisCount++] = axesA[i];
    for (int i = 0; i < 3; ++i) candidateAxes[axisCount++] = axesB[i];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            Vector3 cross = Cross(axesA[i], axesB[j]);
            float len = Length(cross);
            if (len > 1e-6f) {
                candidateAxes[axisCount++] = cross / len;
            }
        }
    }

    const Vector3 centerDelta = centerA - centerB;

    float minOverlap = (std::numeric_limits<float>::max)();
    Vector3 bestAxis{};
    bool found = false;

    for (int i = 0; i < axisCount; ++i) {
        const Vector3& axis = candidateAxes[i];

        float ra = std::abs(Dot(axesA[0], axis)) * halfA.x
                 + std::abs(Dot(axesA[1], axis)) * halfA.y
                 + std::abs(Dot(axesA[2], axis)) * halfA.z;
        float rb = std::abs(Dot(axesB[0], axis)) * halfB.x
                 + std::abs(Dot(axesB[1], axis)) * halfB.y
                 + std::abs(Dot(axesB[2], axis)) * halfB.z;

        float dist = Dot(centerDelta, axis);
        float overlap = (ra + rb) - std::abs(dist);

        if (overlap <= 0.0f) {
            // 分離軸が見つかった -> 衝突していない
            // （この判定だけは axisMode によらず全軸で行う）
            return PenetrationResult{};
        }

        // 水平限定のときは、真上・真下へ押してしまう軸を押し出し候補から外す
        if (axisMode == PenetrationAxis::HorizontalOnly && std::abs(axis.y) > kHorizontalAxisTolerance) {
            continue;
        }

        if (overlap < minOverlap) {
            minOverlap = overlap;
            bestAxis = (dist < 0.0f) ? -axis : axis;
            found = true;
        }
    }

    if (!found) return PenetrationResult{};

    PenetrationResult result;
    result.hit = true;
    result.normal = bestAxis;
    result.depth = minOverlap;

    if (axisMode == PenetrationAxis::HorizontalOnly) {
        // わずかに残るY成分を落として完全な水平にする。
        // 水平面へ倒すと軸方向の長さが縮むので、元の軸へ射影した押し出し量が
        // depth のままになるよう距離を割り戻す
        Vector3 flat{ result.normal.x, 0.0f, result.normal.z };
        const float flatLength = Length(flat);
        if (flatLength <= 1e-4f) return PenetrationResult{};
        result.normal = flat / flatLength;
        result.depth = minOverlap / flatLength;
    }

    return result;
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
