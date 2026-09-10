#include "GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include <Input/CameraInput.h>
#include <Utility/DeltaTime.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Collider/AABBCollider.h>
#include <World3D/Collider/OBBCollider.h>
#include <GameData/CollisionCategory.h>
#include <GameData/GameSettings.h>
#include <Debugger/GlobalVariables.h>
#include <algorithm>
#include <cmath>
#include <random>
#ifdef _DEBUG
#include <imgui.h>
#endif

namespace {
constexpr float kPi = 3.14159265358979323846f;

// [-1,1]^3 内の乱数方向ベクトル（カメラシェイクのオフセット方向用）
Vector3 RandomUnitVec() {
	static std::mt19937 rng(std::random_device{}());
	static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
	Vector3 v{ dist(rng), dist(rng), dist(rng) };
	float len = Length(v);
	if (len < 1e-5f) return Vector3(0.0f, 0.0f, 0.0f);
	return v * (1.0f / len);
}

// 長さ0のベクトルでもNaNにならない正規化。
// Engine側の Normalize は 0除算をガードしていないので、方向が潰れうる場所ではこちらを使う
Vector3 NormalizeSafe(const Vector3& v, const Vector3& fallback) {
	float len = Length(v);
	if (len < 1e-4f) return fallback;
	return v * (1.0f / len);
}

// 角度差を[-π, π]へ畳む
float ShortestAngleDiff(float delta) {
	float diff = std::fmod(delta, 2.0f * kPi);
	if (diff > kPi) diff -= 2.0f * kPi;
	if (diff < -kPi) diff += 2.0f * kPi;
	return diff;
}

// 角度a→bを最短経路で補間する（±πの折り返しを考慮）
float LerpAngleShortest(float a, float b, float t) {
	return a + ShortestAngleDiff(b - a) * t;
}

// 最短経路の補間に加えて、1回の回転量を maxDelta で制限する
float LerpAngleClamped(float a, float b, float t, float maxDelta) {
	float step = ShortestAngleDiff(b - a) * t;
	return a + std::clamp(step, -maxDelta, maxDelta);
}

// 0→1へ滑らかに立ち上がる重み（境界で微分が0になるので切り替わりが目立たない）
float SmoothStep01(float x) {
	x = std::clamp(x, 0.0f, 1.0f);
	return x * x * (3.0f - 2.0f * x);
}

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

	RegisterParams();
}

void GameCamera::RegisterParams() {
	global_ = &GlobalVariables::GetInstance();
	// 保存済みの設定があれば読み込む
	global_->LoadFile("Camera", name_);

	// 各種パラメータをエディタに登録（未登録なら既定値で追加）
	global_->AddItem(name_, "Distance", baseFollowDistance_);
	global_->AddItem(name_, "DistanceLerpSpeed", distanceLerpSpeed_);
	global_->AddItem(name_, "BaseHeight", baseHeight_);
	global_->AddItem(name_, "MinDistance", minDistance_);
	global_->AddItem(name_, "SensitivityX", sensitivityX_);
	global_->AddItem(name_, "SensitivityY", sensitivityY_);
	global_->AddItem(name_, "PitchLimit", pitchLimit_);
	global_->AddItem(name_, "AutoRotateEnabled", autoRotateEnabled_);
	global_->AddItem(name_, "AutoRotateDelay", autoRotateDelay_);
	global_->AddItem(name_, "AutoRotateSpeed", autoRotateSpeed_);
	global_->AddItem(name_, "AutoRotateMoveSpeed", autoRotateMoveSpeed_);
	global_->AddItem(name_, "FollowFeedForward", followFeedForward_);
	global_->AddItem(name_, "PositionLagSpeed", positionLagSpeed_);
	global_->AddItem(name_, "LookLagSpeed", lookLagSpeed_);
	global_->AddItem(name_, "LookForwardOffset", lookForwardOffset_);
	global_->AddItem(name_, "LookHeight", lookHeight_);
	global_->AddItem(name_, "LookPitchDip", lookPitchDip_);
	global_->AddItem(name_, "LookTargetLagSpeed", lookTargetLagSpeed_);
	global_->AddItem(name_, "CollisionMargin", collisionMargin_);
	global_->AddItem(name_, "CollisionMinDist", collisionMinDist_);
	global_->AddItem(name_, "CollisionRadius", collisionRadius_);
	global_->AddItem(name_, "CollisionInSpeed", collisionInSpeed_);
	global_->AddItem(name_, "CollisionOutSpeed", collisionOutSpeed_);
	global_->AddItem(name_, "LockOnDistanceMul", lockOnDistanceMul_);
	global_->AddItem(name_, "LockOnDistanceMin", lockOnDistanceMin_);
	global_->AddItem(name_, "LockOnDistanceMax", lockOnDistanceMax_);
	global_->AddItem(name_, "LockOnHeight", lockOnHeight_);
	global_->AddItem(name_, "LockOnRightOffset", lockOnRightOffset_);
	global_->AddItem(name_, "LockOnLagSpeed", lockOnLagSpeed_);
	global_->AddItem(name_, "LockOnYawSpeed", lockOnYawSpeed_);
	global_->AddItem(name_, "LockOnYawMaxSpeed", lockOnYawMaxSpeed_);
	global_->AddItem(name_, "LockOnYawDeadZone", lockOnYawDeadZone_);

	global_->AddItem(name_, "FramingEnabled", framingEnabled_);
	global_->AddItem(name_, "FramingSafeRatio", framingSafeRatio_);
	global_->AddItem(name_, "FramingLookWeight", framingLookWeight_);
	global_->AddItem(name_, "FramingMaxLookOffset", framingMaxLookOffset_);
	global_->AddItem(name_, "FramingMaxDistanceAdd", framingMaxDistanceAdd_);
	global_->AddItem(name_, "FramingSafetyEnabled", framingSafetyEnabled_);
	global_->AddItem(name_, "FramingClampRatio", framingClampRatio_);

	global_->AddItem(name_, "FovNormal", fovNormal_);
	global_->AddItem(name_, "FovDash", fovDash_);
	global_->AddItem(name_, "FovSpeedMin", fovSpeedMin_);
	global_->AddItem(name_, "FovSpeedMax", fovSpeedMax_);
	global_->AddItem(name_, "FovLerpSpeed", fovLerpSpeed_);
	global_->AddItem(name_, "FovPunchDecay", fovPunchDecay_);
	global_->AddItem(name_, "AttackDistanceScale", attackDistanceScale_);
	global_->AddItem(name_, "ActionZoomSpeed", actionZoomSpeed_);

	global_->AddItem(name_, "EnemyFramingEnabled", enemyFramingEnabled_);
	global_->AddItem(name_, "EnemyFramingEdge", enemyFramingEdge_);
	global_->AddItem(name_, "EnemyFramingYawSpeed", enemyFramingYawSpeed_);

	global_->AddItem(name_, "BattleStateEnabled", battleStateEnabled_);
	global_->AddItem(name_, "BattleDistanceScale", battleDistanceScale_);
	global_->AddItem(name_, "BattleFovAdd", battleFovAdd_);
	global_->AddItem(name_, "BattleBlendSpeed", battleBlendSpeed_);

	global_->AddItem(name_, "ShakeMaxOffset", shakeMaxOffset_);
	global_->AddItem(name_, "ShakeDecayRate", shakeDecayRate_);
	global_->AddItem(name_, "ShakeHitTrauma", shakeHitTrauma_);
	global_->AddItem(name_, "ShakeLandTrauma", shakeLandTrauma_);
	global_->AddItem(name_, "ShakeLandSpeedThreshold", shakeLandSpeedThreshold_);

	// 基準距離は実行時状態(distance_)の初期値としても採用する
	distance_ = global_->GetValueRef<float>(name_, "Distance");
	ApplyParams();

	// 被弾/着地の差分検出の初期値をそろえておく（初回に誤発火させない）
	if (player_) {
		prevHp_ = player_->GetHp();
		prevOnGround_ = player_->GetOnGround();
	}
}

void GameCamera::ApplyParams() {
	if (!global_) return;

	baseFollowDistance_ = global_->GetValueRef<float>(name_, "Distance");
	distanceLerpSpeed_ = global_->GetValueRef<float>(name_, "DistanceLerpSpeed");
	baseHeight_ = global_->GetValueRef<float>(name_, "BaseHeight");
	minDistance_ = global_->GetValueRef<float>(name_, "MinDistance");
	sensitivityX_ = global_->GetValueRef<float>(name_, "SensitivityX");
	sensitivityY_ = global_->GetValueRef<float>(name_, "SensitivityY");
	pitchLimit_ = global_->GetValueRef<float>(name_, "PitchLimit");
	autoRotateEnabled_ = global_->GetValueRef<bool>(name_, "AutoRotateEnabled");
	autoRotateDelay_ = global_->GetValueRef<float>(name_, "AutoRotateDelay");
	autoRotateSpeed_ = global_->GetValueRef<float>(name_, "AutoRotateSpeed");
	autoRotateMoveSpeed_ = global_->GetValueRef<float>(name_, "AutoRotateMoveSpeed");
	followFeedForward_ = global_->GetValueRef<float>(name_, "FollowFeedForward");
	positionLagSpeed_ = global_->GetValueRef<float>(name_, "PositionLagSpeed");
	lookLagSpeed_ = global_->GetValueRef<float>(name_, "LookLagSpeed");
	lookForwardOffset_ = global_->GetValueRef<float>(name_, "LookForwardOffset");
	lookHeight_ = global_->GetValueRef<float>(name_, "LookHeight");
	lookPitchDip_ = global_->GetValueRef<float>(name_, "LookPitchDip");
	lookTargetLagSpeed_ = global_->GetValueRef<float>(name_, "LookTargetLagSpeed");
	collisionMargin_ = global_->GetValueRef<float>(name_, "CollisionMargin");
	collisionMinDist_ = global_->GetValueRef<float>(name_, "CollisionMinDist");
	collisionRadius_ = global_->GetValueRef<float>(name_, "CollisionRadius");
	collisionInSpeed_ = global_->GetValueRef<float>(name_, "CollisionInSpeed");
	collisionOutSpeed_ = global_->GetValueRef<float>(name_, "CollisionOutSpeed");
	lockOnDistanceMul_ = global_->GetValueRef<float>(name_, "LockOnDistanceMul");
	lockOnDistanceMin_ = global_->GetValueRef<float>(name_, "LockOnDistanceMin");
	lockOnDistanceMax_ = global_->GetValueRef<float>(name_, "LockOnDistanceMax");
	lockOnHeight_ = global_->GetValueRef<float>(name_, "LockOnHeight");
	lockOnRightOffset_ = global_->GetValueRef<float>(name_, "LockOnRightOffset");
	lockOnLagSpeed_ = global_->GetValueRef<float>(name_, "LockOnLagSpeed");
	lockOnYawSpeed_ = global_->GetValueRef<float>(name_, "LockOnYawSpeed");
	lockOnYawMaxSpeed_ = global_->GetValueRef<float>(name_, "LockOnYawMaxSpeed");
	lockOnYawDeadZone_ = global_->GetValueRef<float>(name_, "LockOnYawDeadZone");
	framingEnabled_ = global_->GetValueRef<bool>(name_, "FramingEnabled");
	framingSafeRatio_ = global_->GetValueRef<float>(name_, "FramingSafeRatio");
	framingLookWeight_ = global_->GetValueRef<float>(name_, "FramingLookWeight");
	framingMaxLookOffset_ = global_->GetValueRef<float>(name_, "FramingMaxLookOffset");
	framingMaxDistanceAdd_ = global_->GetValueRef<float>(name_, "FramingMaxDistanceAdd");
	framingSafetyEnabled_ = global_->GetValueRef<bool>(name_, "FramingSafetyEnabled");
	framingClampRatio_ = global_->GetValueRef<float>(name_, "FramingClampRatio");
	fovNormal_ = global_->GetValueRef<float>(name_, "FovNormal");
	fovDash_ = global_->GetValueRef<float>(name_, "FovDash");
	fovSpeedMin_ = global_->GetValueRef<float>(name_, "FovSpeedMin");
	fovSpeedMax_ = global_->GetValueRef<float>(name_, "FovSpeedMax");
	fovLerpSpeed_ = global_->GetValueRef<float>(name_, "FovLerpSpeed");
	fovPunchDecay_ = global_->GetValueRef<float>(name_, "FovPunchDecay");
	attackDistanceScale_ = global_->GetValueRef<float>(name_, "AttackDistanceScale");
	actionZoomSpeed_ = global_->GetValueRef<float>(name_, "ActionZoomSpeed");
	enemyFramingEnabled_ = global_->GetValueRef<bool>(name_, "EnemyFramingEnabled");
	enemyFramingEdge_ = global_->GetValueRef<float>(name_, "EnemyFramingEdge");
	enemyFramingYawSpeed_ = global_->GetValueRef<float>(name_, "EnemyFramingYawSpeed");
	battleStateEnabled_ = global_->GetValueRef<bool>(name_, "BattleStateEnabled");
	battleDistanceScale_ = global_->GetValueRef<float>(name_, "BattleDistanceScale");
	battleFovAdd_ = global_->GetValueRef<float>(name_, "BattleFovAdd");
	battleBlendSpeed_ = global_->GetValueRef<float>(name_, "BattleBlendSpeed");
	shakeMaxOffset_ = global_->GetValueRef<float>(name_, "ShakeMaxOffset");
	shakeDecayRate_ = global_->GetValueRef<float>(name_, "ShakeDecayRate");
	shakeHitTrauma_ = global_->GetValueRef<float>(name_, "ShakeHitTrauma");
	shakeLandTrauma_ = global_->GetValueRef<float>(name_, "ShakeLandTrauma");
	shakeLandSpeedThreshold_ = global_->GetValueRef<float>(name_, "ShakeLandSpeedThreshold");
}

void GameCamera::AddFovPunch(float add) {
	// 連続で呼ばれても足し込まない。ダッシュを連打したときに画角が際限なく広がるのを防ぐ
	fovPunch_ = (std::max)(fovPunch_, add);
}

void GameCamera::AddShake(float trauma) {
	shakeTrauma_ = std::clamp(shakeTrauma_ + trauma, 0.0f, 1.0f);
}

void GameCamera::UpdateShake(float dt) {
	// ===== トリガー検出（既存システムへの結合を避け、Player状態の差分で判定） =====
	if (player_) {
		// 被弾：HPが減ったフレーム
		int32_t hp = player_->GetHp();
		if (hp < prevHp_) {
			AddShake(shakeHitTrauma_);
		}
		prevHp_ = hp;

		// 着地：空中→接地に変化し、かつ直前の落下速度が閾値以上
		bool onGround = player_->GetOnGround();
		float vy = player_->GetVelocity().y;
		float fallSpeed = (vy < 0.0f) ? -vy : 0.0f;
		if (onGround && !prevOnGround_ && prevFallSpeed_ >= shakeLandSpeedThreshold_) {
			AddShake(shakeLandTrauma_);
		}
		prevOnGround_ = onGround;
		prevFallSpeed_ = fallSpeed;
	}

	// ===== 減衰 =====
	shakeTrauma_ = std::clamp(shakeTrauma_ - shakeDecayRate_ * dt, 0.0f, 1.0f);

	// ===== オフセット算出（揺れはtrauma^2に比例させ、小さな揺れを抑える） =====
	float amp = shakeMaxOffset_ * shakeTrauma_ * shakeTrauma_;
	shakeOffset_ = (amp > 1e-5f) ? RandomUnitVec() * amp : Vector3(0.0f, 0.0f, 0.0f);
}

void GameCamera::UpdateFovAndZoom(float dt) {
	// ⑨ Action Camera：攻撃中は寄る。倍率を滑らかに補間して急なズームを避ける
	float targetScale = player_->IsAttack() ? attackDistanceScale_ : 1.0f;
	float zoomT = 1.0f - std::exp(-actionZoomSpeed_ * dt);
	actionZoomScale_ += (targetScale - actionZoomScale_) * zoomT;

	// ⑧ FOV変化：水平速度が上がるほど画角を広げる
	Vector3 vel = player_->GetVelocity();
	float speed = std::sqrt(vel.x * vel.x + vel.z * vel.z);
	float denom = (std::max)(fovSpeedMax_ - fovSpeedMin_, 1e-3f);
	float speedT = std::clamp((speed - fovSpeedMin_) / denom, 0.0f, 1.0f);
	float targetFov = fovNormal_ + (fovDash_ - fovNormal_) * speedT;
	// ⑫ Battle状態では画角を少し広げる
	targetFov += battleFovAdd_ * battleBlend_;
	// ダッシュ開始などの単発の「蹴り」。速度由来の変化に上乗せして減衰させる
	targetFov += fovPunch_;
	fovPunch_ *= std::exp(-fovPunchDecay_ * dt);

	float fovT = 1.0f - std::exp(-fovLerpSpeed_ * dt);
	float fov = GetFovY() + (targetFov - GetFovY()) * fovT;
	SetFovY(fov);
}

void GameCamera::UpdateCameraState(float dt) {
	// ロックオン中、または画面内に敵が見えていればBattle
	bool inBattle = battleStateEnabled_ &&
		(lockOn_->IsLockOn() || lockOn_->GetBestVisibleTarget() != nullptr);
	state_ = inBattle ? State::Battle : State::Normal;

	// ブレンド値を滑らかに寄せる（急な距離/FOV変化を避ける）
	float target = inBattle ? 1.0f : 0.0f;
	float t = 1.0f - std::exp(-battleBlendSpeed_ * dt);
	battleBlend_ += (target - battleBlend_) * t;
}

void GameCamera::SetMode(Mode mode) {
	if (mode_ == mode) return;

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	// 補間の状態は衝突補正前の位置で持っているので、構図の逆算にもそちらを使う。
	// 表示位置（壁で引き寄せられた位置）から逆算すると、壁際でモードを切り替えたときに構図が飛ぶ
	Vector3 camPos = idealPosInitialized_ ? idealPos_ : GetTranslate();

	// ===== LockOn → Free =====
	if (mode_ == Mode::LockOn && mode == Mode::Free) {
		Vector3 toCam = camPos - playerPos;

		float horizDist = std::sqrt(toCam.x * toCam.x + toCam.z * toCam.z);
		float adjustedY = toCam.y - baseHeight_;

		distance_ = std::sqrt(horizDist * horizDist + adjustedY * adjustedY);
		yaw_ = std::atan2(toCam.x, toCam.z);
		pitch_ = std::atan2(adjustedY, horizDist);

		// 注視オフセットをリセット：前方バイアスなしから滑らかに立ち上げる
		smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	// ===== Free → LockOn =====
	if (mode_ == Mode::Free && mode == Mode::LockOn) {
		// yaw_ は「プレイヤーから見たカメラの方位」で両モード共通なので、そのまま引き継げばOK。
		// LockOn側がここから敵の反対側へ回していく
	}

	mode_ = mode;
}

void GameCamera::ApplyFollowPosition(const Vector3& pivot, const Vector3& desiredPos, float lagSpeed, float dt) {
	if (!idealPosInitialized_) {
		idealPos_ = desiredPos;
		prevPivot_ = pivot;
		idealPosInitialized_ = true;
	}

	// ピボットの移動分を先に加えてから、残差だけを補間する（フィードフォワード）。
	// 指数補間だけだと「目標の速度 / lagSpeed」の定常誤差がずっと残るため、
	// 後退中や敵を打ち上げた時にカメラが置いていかれ、プレイヤーが画面外へ抜けていた
	idealPos_ += (pivot - prevPivot_) * followFeedForward_;
	prevPivot_ = pivot;

	float t = 1.0f - std::exp(-lagSpeed * dt);
	idealPos_ = Lerp(idealPos_, desiredPos, t);

	// 壁にめり込まないよう手前へ引き寄せる。
	// 補正結果は表示位置にだけ効かせ、補間の状態(idealPos_)には書き戻さない。
	// 書き戻すと「壁の奥へ補間する→手前へ引き戻される」を毎フレーム繰り返して壁際でガタつく
	const Vector3 seg = idealPos_ - pivot;
	const float segLen = Length(seg);
	if (segLen < 1e-4f) {
		GetTranslate() = idealPos_;
		return;
	}
	const Vector3 dir = seg * (1.0f / segLen);

	// 寄せ具合は「割合」で持って非対称に補間する。遮蔽されたら即座に寄り、晴れたらゆっくり戻す。
	// 位置そのものを平滑化すると、理想距離が変わったときの追従まで鈍ってしまう
	const float allowedRatio = CalcAllowedDistance(pivot, dir, segLen) / segLen;
	const float speed = (allowedRatio < collisionRatio_) ? collisionInSpeed_ : collisionOutSpeed_;
	collisionRatio_ += (allowedRatio - collisionRatio_) * (1.0f - std::exp(-speed * dt));
	collisionRatio_ = std::clamp(collisionRatio_, 0.0f, 1.0f);

	GetTranslate() = pivot + dir * (segLen * collisionRatio_);
}

void GameCamera::ApplySmoothLookAt(const Vector3& lookTarget) {
	if (!lookTargetInitialized_) {
		// 初回は遅延させず現在の注視点をそのまま採用する
		smoothedLookTarget_ = lookTarget;
		lookTargetInitialized_ = true;
	} else {
		// Free/LockOnの切り替え等で注視点が急に変わっても瞬間的に視点が飛ばないよう追従させる
		float t = 1.0f - std::exp(-lookTargetLagSpeed_ * DeltaTime::GetDeltaTime());
		smoothedLookTarget_ = Lerp(smoothedLookTarget_, lookTarget, t);
	}

	// カメラと注視点が重なると LookAt の正規化がNaNになり、以降カメラが壊れる。
	// その1フレームは向きを据え置く
	if (Length(smoothedLookTarget_ - GetTranslate()) < 1e-3f) return;

	LookAt(smoothedLookTarget_);
}

float GameCamera::CalcAllowedDistance(const Vector3& pivot, const Vector3& dir, float maxDist) const {
	if (maxDist < 1e-4f) return maxDist;

	// カメラの当たり半径ぶん壁を膨らませて判定する（点のレイのままだと、壁の角をかすめる度に
	// ヒット/ノーヒットが入れ替わって引き寄せ距離がバタつく）
	const Vector3 radius(collisionRadius_, collisionRadius_, collisionRadius_);

	float nearestT = maxDist;
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
			Vector3 half = (aabb->GetMax() - aabb->GetMin()) * 0.5f + radius;
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
			if (IntersectSegmentSlabs(localOrigin, localDir, nearestT, obb->GetWorldHalfExtents() + radius, tHit)) {
				nearestT = tHit;
				hit = true;
			}
		}
	}

	if (!hit) return maxDist;

	// 理想距離より近い側に、最小距離を下回らない範囲で寄せる
	const float lo = (std::min)(collisionMinDist_, maxDist);
	return std::clamp(nearestT - collisionMargin_, lo, maxDist);
}

float GameCamera::CalcFramingDistanceAdd(const Vector3& camPos, const Vector3& lookTarget, const Vector3& anchorA, const Vector3& anchorB) const {
	// 視線方向の基底（ワールドの上を基準にするのでロールは入らない）
	const Vector3 fwd = NormalizeSafe(lookTarget - camPos, Vector3(0.0f, 0.0f, 1.0f));
	const Vector3 right = NormalizeSafe(Cross(Vector3(0.0f, 1.0f, 0.0f), fwd), Vector3(1.0f, 0.0f, 0.0f));
	const Vector3 up = Cross(fwd, right);

	// GetFovY は縦方向の画角。横は aspect ぶん広い
	const float halfFovY = GetFovY() * 0.5f;
	const float halfFovX = std::atan(std::tan(halfFovY) * GetAspectRate());
	const float tanV = std::tan(halfFovY * framingSafeRatio_);
	const float tanH = std::tan(halfFovX * framingSafeRatio_);
	if (tanV < 1e-4f || tanH < 1e-4f) return 0.0f;

	// 視線に沿って delta だけ下がると、横のずれはそのままで奥行きだけ delta 増える。
	// |ずれ| / (奥行き + delta) <= tan(安全画角) を満たす最小の delta を全アンカーぶん求める
	float need = 0.0f;
	const Vector3 anchors[2] = { anchorA, anchorB };
	for (const Vector3& anchor : anchors) {
		const Vector3 v = anchor - camPos;
		const float depth = Dot(v, fwd);
		const float needH = std::abs(Dot(v, right)) / tanH - depth;
		const float needV = std::abs(Dot(v, up)) / tanV - depth;
		need = (std::max)(need, (std::max)(needH, needV));
	}

	return std::clamp(need, 0.0f, framingMaxDistanceAdd_);
}

void GameCamera::ClampIntoView(const Vector3& anchor) {
	const Vector3 v = anchor - GetTranslate();

	// LookAt が入れたオイラー角（ロールなし）からカメラの基底を組み直す
	const float pitch = GetRotate().x;
	const float yaw = GetRotate().y;
	const float cp = std::cos(pitch);
	const float sp = std::sin(pitch);
	const float cy = std::cos(yaw);
	const float sy = std::sin(yaw);
	const Vector3 fwd(cp * sy, -sp, cp * cy);
	const Vector3 right(cy, 0.0f, -sy);
	const Vector3 up = Cross(fwd, right);

	const float depth = Dot(v, fwd);
	// 真横・背後にいる構図は回しても収まらないので触らない
	if (depth < 0.2f) return;

	const float halfFovY = GetFovY() * 0.5f;
	const float halfFovX = std::atan(std::tan(halfFovY) * GetAspectRate());
	const float limitV = halfFovY * framingClampRatio_;
	const float limitH = halfFovX * framingClampRatio_;

	// 縦：カメラの右軸まわりの回転＝pitch そのもの（pitchは下向きが正）
	const float angV = std::atan(Dot(v, up) / depth);
	if (angV > limitV) {
		GetRotate().x -= (angV - limitV);
	} else if (angV < -limitV) {
		GetRotate().x -= (angV + limitV);
	}

	// 横：yaw はワールドのY軸まわりなので、画面上の横移動量は cos(pitch) 倍になる。その分だけ多く回す
	const float angH = std::atan(Dot(v, right) / depth);
	const float yawScale = 1.0f / (std::max)(std::cos(GetRotate().x), 0.2f);
	if (angH > limitH) {
		GetRotate().y += (angH - limitH) * yawScale;
	} else if (angH < -limitH) {
		GetRotate().y += (angH + limitH) * yawScale;
	}
}

void GameCamera::Update() {
	if (!player_) return;

	// エディタでの変更を毎フレーム反映する
	ApplyParams();

	// ⑫ Normal/Battle状態を先に更新（FOV・距離計算がブレンド値を参照する）
	UpdateCameraState(DeltaTime::GetDeltaTime());

	// FOV・アクションズーム倍率を更新（各モードの距離計算で参照する）
	UpdateFovAndZoom(DeltaTime::GetDeltaTime());

	switch (mode_) {
	case Mode::Free:
		UpdateFree();
		break;

	case Mode::LockOn:
		UpdateLockOn();
		break;
	}

	// ⑥ 最終保証：構図がどれだけ遅れても、プレイヤーだけは画面内に残るよう向きを詰める。
	// smoothedLookTarget_ には触らないので、この補正が次フレームの補間へ効き戻ることはない
	if (framingSafetyEnabled_) {
		ClampIntoView(player_->GetWorldTransform()->GetTranslation() + Vector3(0.0f, lookHeight_, 0.0f));
	}

	// ⑩ シェイクを反映。補間の連続性を保つため、揺れは行列生成の瞬間だけ加え、
	// 実体の位置(clean)はシェイクを含めない値に戻す（次フレームの補間へ揺れが蓄積しない）
	UpdateShake(DeltaTime::GetDeltaTime());
	const Vector3 cleanPos = GetTranslate();
	GetTranslate() = cleanPos + shakeOffset_;
	BaseCamera::Update();
	GetTranslate() = cleanPos;
}

Vector3 GameCamera::CalcOrbitPosition(const Vector3& pivotPos, float yaw, float pitch, float distance, float height, float rightOffset) const {
	// 水平の「プレイヤー→カメラ」方向と、その画面右方向（= Cross(up, 視線方向)）
	const Vector3 camDir(std::sin(yaw), 0.0f, std::cos(yaw));
	const Vector3 right(-camDir.z, 0.0f, camDir.x);

	Vector3 pos = pivotPos + camDir * (std::cos(pitch) * distance) + right * rightOffset;
	pos.y += height + std::sin(pitch) * distance;
	return pos;
}

Vector3 GameCamera::CalcDesiredPosition(const Vector3& playerPos) const {
	Vector3 desiredPos = CalcOrbitPosition(playerPos, yaw_, pitch_, distance_, baseHeight_, 0.0f);

	// プレイヤーに寄りすぎないよう最低距離を確保する
	Vector3 toCamera = desiredPos - playerPos;
	if (Length(toCamera) < minDistance_) {
		// 距離も高さも0の縮退時は真上へ逃がす（Normalizeのゼロ除算を避ける）
		desiredPos = playerPos + NormalizeSafe(toCamera, Vector3(0.0f, 1.0f, 0.0f)) * minDistance_;
	}
	return desiredPos;
}

void GameCamera::SnapToFollow(const Vector3& playerForward) {
	if (!player_) return;

	// Updateを経由せず呼ばれるため、ここでもエディタ値を反映しておく
	ApplyParams();

	const Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

	// 追従位置は playerPos + (sin(yaw), *, cos(yaw)) * distance なので、
	// 背後（-forward）を向くような yaw を逆算する
	Vector3 forward = playerForward;
	forward.y = 0.0f;
	if (Length(forward) > 1e-4f) {
		forward = Normalize(forward);
		yaw_ = std::atan2(-forward.x, -forward.z);
	}
	pitch_ = 0.0f;

	const Vector3 pivot = playerPos + Vector3(0.0f, lookHeight_, 0.0f);
	const Vector3 desiredPos = CalcDesiredPosition(playerPos);

	// 補間の途中状態を捨てて、この位置・注視点をそのまま採用させる
	idealPos_ = desiredPos;
	prevPivot_ = pivot;
	idealPosInitialized_ = true;

	const Vector3 seg = desiredPos - pivot;
	const float segLen = Length(seg);
	if (segLen > 1e-4f) {
		const Vector3 dir = seg * (1.0f / segLen);
		collisionRatio_ = CalcAllowedDistance(pivot, dir, segLen) / segLen;
		GetTranslate() = pivot + dir * (segLen * collisionRatio_);
	} else {
		collisionRatio_ = 1.0f;
		GetTranslate() = desiredPos;
	}

	smoothedLookOffset_ = Vector3(0.0f, 0.0f, 0.0f);
	lookTargetInitialized_ = false;
	ApplySmoothLookAt(pivot);

	BaseCamera::Update();
}

void GameCamera::UpdateFree() {
	if (lockOn_->IsLockOn()) {
		SetMode(Mode::LockOn);
		return;
	}

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();

	Vector2 stick = cameraInput_->GetStickDirection();

	// 感度と上下反転はタイトルの OPTION から触れる。
	// GlobalVariables 側の sensitivityX_ / Y_ はカメラの作り込みの値なので、
	// プレイヤーの好みはそこへ掛ける倍率として持たせている
	const GameSettings& settings = GameSettings::GetInstance();
	const float sensitivityScale = settings.GetCameraSensitivity();
	const float verticalSign = settings.IsInvertCameraY() ? -1.0f : 1.0f;

	yaw_ += stick.x * sensitivityX_ * sensitivityScale;
	pitch_ -= stick.y * sensitivityY_ * sensitivityScale * verticalSign;

	pitch_ = std::clamp(pitch_, -pitchLimit_, pitchLimit_);

	// ⑤ Auto Rotation：右スティック無入力が続き、かつ移動中なら徐々に背後へ戻す
	const float dt = DeltaTime::GetDeltaTime();
	if (cameraInput_->GetStickMagnitude() > 0.01f) {
		autoRotateTimer_ = 0.0f;
	} else {
		autoRotateTimer_ += dt;

		Vector3 vel = player_->GetVelocity();
		float horizSpeed = std::sqrt(vel.x * vel.x + vel.z * vel.z);

		if (autoRotateEnabled_ && autoRotateTimer_ >= autoRotateDelay_ && horizSpeed >= autoRotateMoveSpeed_) {
			Vector3 forwardH = player_->GetWorldTransform()->GetForward();
			forwardH.y = 0.0f;
			if (Length(forwardH) > 1e-4f) {
				forwardH = Normalize(forwardH);
				// 背後（-forward）を向く yaw（SnapToFollowと同じ逆算）
				float targetYaw = std::atan2(-forwardH.x, -forwardH.z);
				float rotT = 1.0f - std::exp(-autoRotateSpeed_ * dt);
				yaw_ = LerpAngleShortest(yaw_, targetYaw, rotT);
			}
		}
	}

	// ⑪ Enemy Framing：スティック無入力時、敵が画面横端に寄っていたら中央へ向けて軽くYaw補正する
	if (enemyFramingEnabled_ && cameraInput_->GetStickMagnitude() <= 0.01f) {
		if (LockOnTarget* target = lockOn_->GetBestVisibleTarget()) {
			// IsInViewは前方(w>0)のときだけndcを書き込むので、番兵で前方判定する
			Vector3 ndc(0.0f, 0.0f, -1000.0f);
			IsInView(target->GetWorldPosition(), &ndc);
			bool inFront = ndc.z > -100.0f;
			float ax = std::abs(ndc.x);
			if (inFront && ax > enemyFramingEdge_) {
				// 端に寄るほど強く。ndc.xの符号に応じて中央へ寄せる向きにyawを回す
				float denom = (std::max)(1.0f - enemyFramingEdge_, 1e-3f);
				float strength = std::clamp((ax - enemyFramingEdge_) / denom, 0.0f, 1.0f);
				yaw_ -= std::copysign(strength * enemyFramingYawSpeed_ * dt, ndc.x);
			}
		}
	}

	// ⑨⑫ 攻撃時ズームとBattle状態を反映した基準距離へ、実行時距離を滑らかに寄せる
	float battleDistFactor = 1.0f + (battleDistanceScale_ - 1.0f) * battleBlend_;
	float targetDist = baseFollowDistance_ * actionZoomScale_ * battleDistFactor;
	float distT = 1.0f - std::exp(-distanceLerpSpeed_ * dt);
	distance_ += (targetDist - distance_) * distT;

	Vector3 desiredPos = CalcDesiredPosition(playerPos);

	// プレイヤー前方への注視オフセットを遅延追従させる
	// 小さな動きを吸収しつつ持続した移動方向に追従
	Vector3 forward = NormalizeSafe(player_->GetWorldTransform()->GetForward(), Vector3(0.0f, 0.0f, 0.0f));
	float lookT = 1.0f - std::exp(-lookLagSpeed_ * dt);
	smoothedLookOffset_ = Lerp(smoothedLookOffset_, forward * lookForwardOffset_, lookT);

	Vector3 lookTarget = playerPos + Vector3(0, lookHeight_, 0) + smoothedLookOffset_ - Vector3(0, sin(pitch_) * lookPitchDip_, 0);

	ApplyFollowPosition(playerPos + Vector3(0.0f, lookHeight_, 0.0f), desiredPos, positionLagSpeed_, dt);

	ApplySmoothLookAt(lookTarget);
}

void GameCamera::UpdateLockOn() {
	if (!lockOn_->IsLockOn()) {
		SetMode(Mode::Free);
		return;
	}

	const float dt = DeltaTime::GetDeltaTime();

	Vector3 playerPos = player_->GetWorldTransform()->GetTranslation();
	Vector3 enemyPos = lockOn_->GetCurrentTarget()->GetWorldPosition();

	// ===== 方位(yaw)の更新 =====
	// 敵の真下・真上を通ると水平距離が0に近づき、そこから作った方向ベクトルが暴れる
	// （Normalizeはゼロ除算でNaNになる）。毎フレーム方向から構図を作り直すのをやめ、
	// yaw_ を状態として持ち、減速つきで目標方位へ寄せる
	Vector3 flat = enemyPos - playerPos;
	flat.y = 0.0f;
	const float horizDist = Length(flat);

	// 水平距離が近いほど方位の更新権限を落とし、ほぼ真下・真上では凍結する
	const float authority = SmoothStep01(horizDist / (std::max)(lockOnYawDeadZone_, 1e-3f));
	if (authority > 0.0f) {
		// authority>0 は horizDist>0 を保証する
		Vector3 toEnemyDir = flat * (1.0f / horizDist);
		// カメラは敵の反対側（プレイヤーの背後）に置くので、その向きを目標にする
		float targetYaw = std::atan2(-toEnemyDir.x, -toEnemyDir.z);
		float t = (1.0f - std::exp(-lockOnYawSpeed_ * dt)) * authority;
		yaw_ = LerpAngleClamped(yaw_, targetYaw, t, lockOnYawMaxSpeed_ * dt);
	}

	// ロックオン中の基準の仰角は0（高さは lockOnHeight_ が受け持つ）。
	// Freeから引き継いだ傾きはここで滑らかに戻す
	pitch_ += (0.0f - pitch_) * (1.0f - std::exp(-lockOnYawSpeed_ * dt));

	// ===== フレーミング =====
	// この2点が画面に収まっていればよい
	const Vector3 anchorPlayer = playerPos + Vector3(0.0f, lookHeight_, 0.0f);
	const Vector3 anchorTarget = enemyPos + Vector3(0.0f, lookHeight_, 0.0f);

	// 注視点。中点そのままだと敵を打ち上げた時に注視点だけ上がってプレイヤーが下へ押し出されるので、
	// プレイヤーから離れられる距離に上限をつける
	Vector3 lookTarget = Lerp(anchorPlayer, anchorTarget, framingLookWeight_);
	const Vector3 lookOffset = lookTarget - anchorPlayer;
	const float lookOffsetLen = Length(lookOffset);
	if (lookOffsetLen > framingMaxLookOffset_) {
		lookTarget = anchorPlayer + lookOffset * (framingMaxLookOffset_ / lookOffsetLen);
	}

	const float distToEnemy = Length(enemyPos - playerPos);
	float distance = std::clamp(distToEnemy * lockOnDistanceMul_, lockOnDistanceMin_, lockOnDistanceMax_);
	// ⑨ 攻撃中はロックオンでも少し寄る
	distance *= actionZoomScale_;

	if (framingEnabled_) {
		// 基準距離での構図を試算し、2点が安全枠に収まるまで必要なぶんだけ下がる。
		// 下がった量は次フレームの計算に持ち越さない（毎フレーム作り直すので発散しない）
		const Vector3 probe = CalcOrbitPosition(playerPos, yaw_, pitch_, distance, lockOnHeight_, lockOnRightOffset_);
		distance += CalcFramingDistanceAdd(probe, lookTarget, anchorPlayer, anchorTarget);
	}

	// Free と同じ極座標の状態として持つ（モードをまたいでも構図が連続する）
	distance_ = distance;
	const Vector3 cameraPos = CalcOrbitPosition(playerPos, yaw_, pitch_, distance_, lockOnHeight_, lockOnRightOffset_);

	ApplyFollowPosition(anchorPlayer, cameraPos, lockOnLagSpeed_, dt);

	ApplySmoothLookAt(lookTarget);
}
