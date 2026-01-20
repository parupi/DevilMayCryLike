#include "PlayerCollider.h"
#include <3d/Collider/CollisionManager.h>

void PlayerCollider::Initialize(BaseCollider* collider)
{
	collider_ = collider;
}

void PlayerCollider::OnCollisionEnter(BaseCollider* other)
{
}

void PlayerCollider::OnCollisionStay(BaseCollider* other)
{
	currentColliders_.push_back(other);
}

void PlayerCollider::OnCollisionExit(BaseCollider* other)
{
}

void PlayerCollider::Update()
{
	hits_.clear();
	isOnGround_ = false;

	for (BaseCollider* other : currentColliders_) {

		CollisionHit hit{};
		if (!ComputeHit(collider_, other, hit)) {
			continue;
		}

		if (hit.normal.y > 0.5f) {
			isOnGround_ = true;
		}

		hits_.push_back(hit);
	}

	currentColliders_.clear();
}

bool PlayerCollider::ComputeHit(BaseCollider* self, BaseCollider* other, CollisionHit& outHit)
{
	if (self->GetShapeType() == CollisionShapeType::AABB && other->GetShapeType() == CollisionShapeType::AABB) {

		return ComputeAABBvsAABB(
			static_cast<AABBCollider*>(self),
			static_cast<AABBCollider*>(other),
			outHit
		);
	}

	// 今後 Capsule / Sphere を足せる
	return false;
}

bool PlayerCollider::ComputeAABBvsAABB(AABBCollider* a, AABBCollider* b, CollisionHit& outHit)
{
	Vector3 delta = b->transform_->GetWorldPos() - a->transform_->GetWorldPos();

	Vector3 overlap = {
		(a->GetSize().x + b->GetSize().x) / 2.0f - std::abs(delta.x),
		(a->GetSize().y + b->GetSize().y) / 2.0f - std::abs(delta.y),
		(a->GetSize().z + b->GetSize().z) / 2.0f - std::abs(delta.z)
	};

	if (overlap.x <= 0 || overlap.y <= 0 || overlap.z <= 0) {
		return false;
	}

	// 最小侵入軸を選ぶ
	if (overlap.x < overlap.y && overlap.x < overlap.z) {
		outHit.normal = { delta.x < 0 ? -1.0f : 1.0f, 0, 0 };
		outHit.penetration = overlap.x;
	} else if (overlap.y < overlap.z) {
		outHit.normal = { 0, delta.y < 0 ? -1.0f : 1.0f, 0 };
		outHit.penetration = overlap.y;
	} else {
		outHit.normal = { 0, 0, delta.z < 0 ? -1.0f : 1.0f };
		outHit.penetration = overlap.z;
	}

	outHit.other = b;
	return true;
}

//CollisionHit PlayerCollider::CheckGround() const
//{
//    CollisionHit result{};
//
//    for (BaseCollider* other : hitColliders_) {
//        Vector3 normal = AABBCollider::CalculateCollisionNormal(collider_, static_cast<AABBCollider*>(other));
//
//        if (normal.y > 0.5f) {
//            result.hit = true;
//            result.normal = normal;
//            result.other = other;
//            break;
//        }
//    }
//
//    return result;
//}
//
//CollisionHit PlayerCollider::CheckWall(const Vector3& moveDir) const
//{
//    CollisionHit result{};
//
//    for (BaseCollider* other : hitColliders_) {
//        Vector3 normal = AABBCollider::CalculateCollisionNormal(collider_, static_cast<AABBCollider*>(other));
//
//        if (std::abs(normal.y) < 0.1f) {
//            if (Dot(normal, moveDir) < 0.0f) {
//                result.hit = true;
//                result.normal = normal;
//                result.other = other;
//                break;
//            }
//        }
//    }
//
//    return result;
//}

//const std::vector<CollisionHit>& PlayerCollider::GetHits() const
//{
//    return hits_;
//}
//
//bool PlayerCollider::ComputeHit(
//    BaseCollider* self,
//    BaseCollider* other,
//    CollisionHit& outHit
//)
//{
//    if (self->GetShapeType() == CollisionShapeType::AABB &&
//        other->GetShapeType() == CollisionShapeType::AABB) {
//
//        return ComputeAABBvsAABB(
//            static_cast<AABBCollider*>(self),
//            static_cast<AABBCollider*>(other),
//            outHit
//        );
//    }
//
//    // 今後 Capsule / Sphere を足せる
//    return false;
//}