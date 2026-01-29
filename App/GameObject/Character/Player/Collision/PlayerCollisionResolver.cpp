#define NOMINMAX
#include "PlayerCollisionResolver.h"
#include "PlayerCollider.h"
#include <algorithm>

PlayerCollisionResolver::Result PlayerCollisionResolver::Resolve(const std::vector<CollisionHit>& hits, const Vector3& velocity)
{
    Result result{};

    for (const auto& hit : hits) {
        if (hit.normal.y > 0.5f) {
            ResolveGround(hit, result);
        } else if (hit.normal.y < -0.5f) {
            ResolveCeiling(hit, result);
        } else {
            ResolveWall(hit, result, velocity);
        }
    }

    return result;
}

void PlayerCollisionResolver::ResolveGround(const CollisionHit& hit, Result& result)
{
    result.positionOffset.y += hit.normal.y * hit.penetration;
    result.velocityOffset.y = std::max(result.velocityOffset.y, 0.0f);
    result.onGround = true;
}

void PlayerCollisionResolver::ResolveWall(const CollisionHit& hit, Result& result, const Vector3& velocity)
{
    Vector3 horizontalNormal = hit.normal;
    horizontalNormal.y = 0.0f;

    float dot = Dot(velocity, horizontalNormal);
    if (dot < 0.0f) {
        result.velocityOffset -= horizontalNormal * dot;
    }

    result.positionOffset += horizontalNormal * hit.penetration;
}

void PlayerCollisionResolver::ResolveCeiling(const CollisionHit& hit, Result& result)
{
    result.positionOffset.y += hit.normal.y * hit.penetration;

    if (result.velocityOffset.y > 0.0f) {
        result.velocityOffset.y = 0.0f;
    }
}
