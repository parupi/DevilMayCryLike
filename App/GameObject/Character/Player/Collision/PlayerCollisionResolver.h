#pragma once
#include <vector>
#include "math/Vector3.h"

struct CollisionHit;

class PlayerCollisionResolver
{
public:
    struct Result {
        Vector3 positionOffset; // 押し戻し量
        Vector3 velocityOffset; // 速度補正
        bool onGround = false;
    };

    Result Resolve(const std::vector<CollisionHit>& hits, const Vector3& velocity);

private:
    void ResolveGround(const CollisionHit& hit, Result& result);

    void ResolveWall(const CollisionHit& hit, Result& result, const Vector3& velocity);

    void ResolveCeiling(const CollisionHit& hit, Result& result);
};
