#include "GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include <Input/CameraInput.h>
#include <Utility/DeltaTime.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Collider/AABBCollider.h>
#include <World3D/Collider/OBBCollider.h>
#include <GameData/CollisionCategory.h>
#include <algorithm>
#include <cmath>

static constexpr float kCameraBaseHeight = 3.0f;

namespace {
// 線分(origin + dir*t, t∈(0, tMax])と原点中心スラブ領域(±halfExtents)の交差判定（スラブ法）
// origin/dir は判定対象のローカル空間に変換済みであること。dir は単位ベクトル前提（tは距離）
bool IntersectSegmentSlabs(const Vector3& origin, const Vector3& dir, float tMax, const Vector3& halfExtents, float& tHit) {
	float tEnter = 0.0f;
	float tExit = tMax;
	const float o[3] = { origin.x, origin.y, origin.z };
	const float d[3] = { dir.x, dir.y, dir.z };
	const float h[3] = { halfExtents.x, halfExtents.y, halfExtents.z };
	for (int i = 0; i < 3; ++i) {
		if (std::abs(d[i]) < 1e-6f) {
			// 軸に平行: スラブ範囲外なら交差しない
			if (o[i] < -h[i] || o[i] > h[i]) return false;
			continue;
		}
		float t1 = (-h[i] - o[i]) / d[i];
		float t2 = (h[i] - o[i]) / d[i];
		if (t1 > t2) std::swap(t1, t2);
		tEnter = (std::max)(tEnter, t1);
		tExit = (std::min)(tExit, t2);
		if (tEnter > tExit) return false;
	}
	// 始点がすでに内部にある場合は引き寄せ先が無いので対象外
	if (tEnter <= 0.0f) return false;
	tHit = tEnter;
	return true;
}
} // namespace

GameCamera::GameCamera(std::string cameraName)
	: BaseCamera(cameraName) {
}

void GameCamera::Initialize(Player* player, LockOnSystem* lockOn, CameraInput* cameraInput) {
	player_ = player;
	lockOn_ = lockOn;
	cameraInput_ = cameraInput;
}

void GameCamera::SetMode(Mode mode) {
	if (mode_ == mode) return;

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	Vector3 camPos = GetTranslate();

	// ===== LockOn → Free =====
	if (mode_ == Mode::LockOn && mode == Mode::Free) {
		Vector3 toCam = camPos - playerPos;

		float horizDist = std::sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
		float adjustedY = toCam.y - kCameraBaseHeight;

		distance_ = std::sqrt(horizDist * horizDist + adjustedY * adjustedY);
		yaw_ = std::atan2(toCam.x, toCam.z);
		pitch_ = std::atan2(adjustedY, horizDist);

		// 注視オフセットをリセット：前方バイアスなしから滑らかに立ち上げる
		smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	// ===== Free → LockOn =====
	if (mode_ == Mode::Free && mode == Mode::LockOn) {
		// yawだけ引き継げばOK（見た目はそのまま）
	}

	mode_ = mode;
}

void GameCamera::ApplySmoothLookAt(const Vector3& lookTarget) {
	if (!lookTargetInitialized_) {
		// 初回は遅延させず現在の注視点をそのまま採用する
		smoothedLookTarget_ = lookTarget;
		lookTargetInitialized_ = true;
	} else {
		// Free/LockOnの切り替え等で注視点が急に変わっても瞬間的に視点が飛ばないよう追従させる
		float t = 1.0f - std::exp(-8.0f * DeltaTime::GetDeltaTime());
		smoothedLookTarget_ = Lerp(smoothedLookTarget_, lookTarget, t);
	}

	LookAt(smoothedLookTarget_);
}

Vector3 GameCamera::ResolveCameraCollision(const Vector3& pivot, const Vector3& desiredPos) const {
	Vector3 seg = desiredPos - pivot;
	float segLen = Length(seg);
	if (segLen < 1e-4f) return desiredPos;
	Vector3 dir = seg * (1.0f / segLen);

	constexpr float kMargin = 0.4f;  // 遮蔽物の手前に確保する余白
	constexpr float kMinDist = 1.0f; // ピボットに近づきすぎない最小距離

	float nearestT = segLen;
	bool hit = false;

	for (const auto& col : CollisionManager::GetInstance().GetColliders()) {
		if (!col->isAlive) continue;
		// 壁・床などの静的地形のみを遮蔽物として扱う（敵やトリガーでカメラが動くのを防ぐ）
		if (col->category_ != CollisionCategory::Ground) continue;

		float tHit = 0.0f;
		if (col->GetShapeType() == CollisionShapeType::AABB) {
			auto* aabb = static_cast<AABBCollider*>(col.get());
			if (!aabb->GetColliderData().isActive) continue;
			Vector3 center = (aabb->GetMin() + aabb->GetMax()) * 0.5f;
			Vector3 half = (aabb->GetMax() - aabb->GetMin()) * 0.5f;
			if (IntersectSegmentSlabs(pivot - center, dir, nearestT, half, tHit)) {
				nearestT = tHit;
				hit = true;
			}
		} else if (col->GetShapeType() == CollisionShapeType::OBB) {
			auto* obb = static_cast<OBBCollider*>(col.get());
			if (!obb->GetColliderData().isActive) continue;
			// OBBのローカル空間に射影してスラブ判定（axesは正規直交なのでtは距離のまま）
			Vector3 rel = pivot - obb->GetCenter();
			Vector3 localOrigin{ Dot(rel, obb->GetAxis(0)), Dot(rel, obb->GetAxis(1)), Dot(rel, obb->GetAxis(2)) };
			Vector3 localDir{ Dot(dir, obb->GetAxis(0)), Dot(dir, obb->GetAxis(1)), Dot(dir, obb->GetAxis(2)) };
			if (IntersectSegmentSlabs(localOrigin, localDir, nearestT, obb->GetWorldHalfExtents(), tHit)) {
				nearestT = tHit;
				hit = true;
			}
		}
	}

	if (!hit) return desiredPos;

	float dist = (std::max)(nearestT - kMargin, kMinDist);
	return pivot + dir * dist;
}

void GameCamera::Update() {
	if (!player_) return;

	switch (mode_) {
	case Mode::Free:
		UpdateFree();
		break;

	case Mode::LockOn:
		UpdateLockOn();
		break;
	}

	BaseCamera::Update();
}

void GameCamera::UpdateFree() {
	if (lockOn_->IsLockOn()) {
		SetMode(Mode::LockOn);
		return;
	}

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

	Vector2 stick = cameraInput_->GetStickDirection();

	yaw_ += stick.x * sensitivityX;
	pitch_ -= stick.y * sensitivityY;

	float pitchLimit = 1.2f;
	pitch_ = std::clamp(pitch_, -pitchLimit, pitchLimit);

	Vector3 offset;
	offset.x = cos(pitch_) * sin(yaw_) * distance_;
	offset.y = kCameraBaseHeight + sin(pitch_) * distance_;
	offset.z = cos(pitch_) * cos(yaw_) * distance_;

	Vector3 desiredPos = playerPos + offset;

	float minDist = 5.0f;
	Vector3 toCamera = desiredPos - playerPos;
	if (Length(toCamera) < minDist) {
		desiredPos = playerPos + Normalize(toCamera) * minDist;
	}

	// プレイヤー前方への注視オフセットを遅延追従させる
	// 小さな動きを吸収しつつ持続した移動方向に追従
	Vector3 forward = Normalize(player_->GetWorldTransform()->GetForward());
	float lookT = 1.0f - std::exp(-2.5f * DeltaTime::GetDeltaTime());
	smoothedLookOffset_ = Lerp(smoothedLookOffset_, forward * 3.0f, lookT);

	Vector3 lookTarget = playerPos + Vector3(0, 2.0f, 0) + smoothedLookOffset_ - Vector3(0, sin(pitch_) * 4.0f, 0);

	float t = 1.0f - std::exp(-5.0f * DeltaTime::GetDeltaTime());
	GetTranslate() = Lerp(GetTranslate(), desiredPos, t);

	// 壁にめり込まないよう、プレイヤーとの間に遮蔽物があれば手前へ引き寄せる
	GetTranslate() = ResolveCameraCollision(playerPos + Vector3(0.0f, 2.0f, 0.0f), GetTranslate());

	ApplySmoothLookAt(lookTarget);
}

void GameCamera::UpdateLockOn() {
	if (!lockOn_->IsLockOn()) {
		SetMode(Mode::Free);
		return;
	}

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	Vector3 enemyPos = lockOn_->GetCurrentTarget()->GetWorldPosition();

	Vector3 toEnemy = enemyPos - playerPos;
	toEnemy.y = 0.0f;
	toEnemy = Normalize(toEnemy);

	float distToEnemy = Length(enemyPos - playerPos);

	float distance = std::clamp(distToEnemy * 1.2f, 12.0f, 25.0f);

	const float height = 8.0f;

	Vector3 right = Normalize(Cross(Vector3(0, 1, 0), toEnemy));

	Vector3 cameraPos = playerPos - toEnemy * distance + right * 3.0f;

	cameraPos.y += height;

	Vector3 lookTarget = (playerPos + enemyPos) * 0.5f + Vector3(0, 2.0f, 0);

	float t = 1.0f - std::exp(-5.0f * DeltaTime::GetDeltaTime());
	GetTranslate() = Lerp(GetTranslate(), cameraPos, t);

	// 壁にめり込まないよう、プレイヤーとの間に遮蔽物があれば手前へ引き寄せる
	GetTranslate() = ResolveCameraCollision(playerPos + Vector3(0.0f, 2.0f, 0.0f), GetTranslate());

	ApplySmoothLookAt(lookTarget);

	// Free復帰用
	yaw_ = std::atan2(toEnemy.x, toEnemy.z);
}