#include "FootstepTracker.h"

#include "World3D/Object/Model/Animation/AnimationPlayer.h"

#include <cmath>

namespace {
	// 足音のイベントタグ。EditScene のスキンテストが使っているものと同じ
	const char* const kFootstepTag = "footstep";

	// 1フレームでこれ以上動いたら、歩いたのではなく位置が飛んだと見なす。
	// ワープやリスポーンで一気に足音が連打されるのを防ぐ
	constexpr float kTeleportDistance = 5.0f;
}

bool FootstepTracker::Update(const Vector3& worldPosition, bool grounded, const AnimationPlayer* animation)
{
	// 浮いている間は刻まない。着地音はジャンプ側が別に鳴らす
	if (!grounded) {
		previousPosition_ = worldPosition;
		hasPrevious_ = true;
		travelled_ = 0.0f;
		return false;
	}

	// アニメーションにイベントが仕込まれていれば、そちらが正確
	if (animation && animation->HasEvents()) {
		previousPosition_ = worldPosition;
		hasPrevious_ = true;
		travelled_ = 0.0f;
		if (!animation->WasEventFired(kFootstepTag)) { return false; }
		++stepCount_;
		return true;
	}

	if (!hasPrevious_) {
		previousPosition_ = worldPosition;
		hasPrevious_ = true;
		return false;
	}

	// 上下の動きは歩幅に数えない（坂や段差で歩調が変わってしまう）
	const Vector3 delta = worldPosition - previousPosition_;
	const float distance = std::sqrt(delta.x * delta.x + delta.z * delta.z);
	previousPosition_ = worldPosition;

	if (distance > kTeleportDistance) {
		travelled_ = 0.0f;
		return false;
	}

	travelled_ += distance;
	if (travelled_ < strideLength_) { return false; }

	// 余りは持ち越す。切り捨てると、速く走るほど歩調が遅れていく
	travelled_ -= strideLength_;
	++stepCount_;
	return true;
}

void FootstepTracker::Reset()
{
	travelled_ = 0.0f;
	hasPrevious_ = false;
}
