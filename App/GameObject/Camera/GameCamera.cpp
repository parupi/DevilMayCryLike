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
		Vector3 v{dist(rng), dist(rng), dist(rng)};
		float len = Length(v);
		if (len < 1e-5f) return Vector3(0.0f, 0.0f, 0.0f);
		return v * (1.0f / len);
	}

	// 角度a→bを最短経路で補間する（±πの折り返しを考慮）
	float LerpAngleShortest(float a, float b, float t) {
		float diff = std::fmod(b - a, 2.0f * kPi);
		if (diff > kPi) diff -= 2.0f * kPi;
		if (diff < -kPi) diff += 2.0f * kPi;
		return a + diff * t;
	}

	// 線分(origin + dir*t, t∈(0, tMax])と原点中心スラブ領域(±halfExtents)の交差判定（スラブ法）
	// origin/dir は判定対象のローカル空間に変換済みであること。dir は単位ベクトル前提（tは距離）
	bool IntersectSegmentSlabs(const Vector3& origin, const Vector3& dir, float tMax, const Vector3& halfExtents, float& tHit) {
		float tEnter = 0.0f;
		float tExit = tMax;
		const float o[3] = {origin.x, origin.y, origin.z};
		const float d[3] = {dir.x, dir.y, dir.z};
		const float h[3] = {halfExtents.x, halfExtents.y, halfExtents.z};
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
	global_->AddItem(name_, "PositionLagSpeed", positionLagSpeed_);
	global_->AddItem(name_, "LookLagSpeed", lookLagSpeed_);
	global_->AddItem(name_, "LookForwardOffset", lookForwardOffset_);
	global_->AddItem(name_, "LookHeight", lookHeight_);
	global_->AddItem(name_, "LookPitchDip", lookPitchDip_);
	global_->AddItem(name_, "LookTargetLagSpeed", lookTargetLagSpeed_);
	global_->AddItem(name_, "CollisionMargin", collisionMargin_);
	global_->AddItem(name_, "CollisionMinDist", collisionMinDist_);
	global_->AddItem(name_, "LockOnDistanceMul", lockOnDistanceMul_);
	global_->AddItem(name_, "LockOnDistanceMin", lockOnDistanceMin_);
	global_->AddItem(name_, "LockOnDistanceMax", lockOnDistanceMax_);
	global_->AddItem(name_, "LockOnHeight", lockOnHeight_);
	global_->AddItem(name_, "LockOnRightOffset", lockOnRightOffset_);
	global_->AddItem(name_, "LockOnLagSpeed", lockOnLagSpeed_);

	global_->AddItem(name_, "FovNormal", fovNormal_);
	global_->AddItem(name_, "FovDash", fovDash_);
	global_->AddItem(name_, "FovSpeedMin", fovSpeedMin_);
	global_->AddItem(name_, "FovSpeedMax", fovSpeedMax_);
	global_->AddItem(name_, "FovLerpSpeed", fovLerpSpeed_);
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
	positionLagSpeed_ = global_->GetValueRef<float>(name_, "PositionLagSpeed");
	lookLagSpeed_ = global_->GetValueRef<float>(name_, "LookLagSpeed");
	lookForwardOffset_ = global_->GetValueRef<float>(name_, "LookForwardOffset");
	lookHeight_ = global_->GetValueRef<float>(name_, "LookHeight");
	lookPitchDip_ = global_->GetValueRef<float>(name_, "LookPitchDip");
	lookTargetLagSpeed_ = global_->GetValueRef<float>(name_, "LookTargetLagSpeed");
	collisionMargin_ = global_->GetValueRef<float>(name_, "CollisionMargin");
	collisionMinDist_ = global_->GetValueRef<float>(name_, "CollisionMinDist");
	lockOnDistanceMul_ = global_->GetValueRef<float>(name_, "LockOnDistanceMul");
	lockOnDistanceMin_ = global_->GetValueRef<float>(name_, "LockOnDistanceMin");
	lockOnDistanceMax_ = global_->GetValueRef<float>(name_, "LockOnDistanceMax");
	lockOnHeight_ = global_->GetValueRef<float>(name_, "LockOnHeight");
	lockOnRightOffset_ = global_->GetValueRef<float>(name_, "LockOnRightOffset");
	lockOnLagSpeed_ = global_->GetValueRef<float>(name_, "LockOnLagSpeed");
	fovNormal_ = global_->GetValueRef<float>(name_, "FovNormal");
	fovDash_ = global_->GetValueRef<float>(name_, "FovDash");
	fovSpeedMin_ = global_->GetValueRef<float>(name_, "FovSpeedMin");
	fovSpeedMax_ = global_->GetValueRef<float>(name_, "FovSpeedMax");
	fovLerpSpeed_ = global_->GetValueRef<float>(name_, "FovLerpSpeed");
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
	Vector3 camPos = GetTranslate();

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
		float t = 1.0f - std::exp(-lookTargetLagSpeed_ * DeltaTime::GetDeltaTime());
		smoothedLookTarget_ = Lerp(smoothedLookTarget_, lookTarget, t);
	}

	LookAt(smoothedLookTarget_);
}

Vector3 GameCamera::ResolveCameraCollision(const Vector3& pivot, const Vector3& desiredPos) const {
	Vector3 seg = desiredPos - pivot;
	float segLen = Length(seg);
	if (segLen < 1e-4f) return desiredPos;
	Vector3 dir = seg * (1.0f / segLen);

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
			Vector3 localOrigin{Dot(rel, obb->GetAxis(0)), Dot(rel, obb->GetAxis(1)), Dot(rel, obb->GetAxis(2))};
			Vector3 localDir{Dot(dir, obb->GetAxis(0)), Dot(dir, obb->GetAxis(1)), Dot(dir, obb->GetAxis(2))};
			if (IntersectSegmentSlabs(localOrigin, localDir, nearestT, obb->GetWorldHalfExtents(), tHit)) {
				nearestT = tHit;
				hit = true;
			}
		}
	}

	if (!hit) return desiredPos;

	float dist = (std::max)(nearestT - collisionMargin_, collisionMinDist_);
	return pivot + dir * dist;
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

	// ⑩ シェイクを反映。補間の連続性を保つため、揺れは行列生成の瞬間だけ加え、
	// 実体の位置(clean)はシェイクを含めない値に戻す（次フレームの補間へ揺れが蓄積しない）
	UpdateShake(DeltaTime::GetDeltaTime());
	const Vector3 cleanPos = GetTranslate();
	GetTranslate() = cleanPos + shakeOffset_;
	BaseCamera::Update();
	GetTranslate() = cleanPos;
}

Vector3 GameCamera::CalcDesiredPosition(const Vector3& playerPos) const {
	Vector3 offset;
	offset.x = std::cos(pitch_) * std::sin(yaw_) * distance_;
	offset.y = baseHeight_ + std::sin(pitch_) * distance_;
	offset.z = std::cos(pitch_) * std::cos(yaw_) * distance_;

	Vector3 desiredPos = playerPos + offset;

	// プレイヤーに寄りすぎないよう最低距離を確保する
	Vector3 toCamera = desiredPos - playerPos;
	if (Length(toCamera) < minDistance_) {
		desiredPos = playerPos + Normalize(toCamera) * minDistance_;
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
	GetTranslate() = ResolveCameraCollision(pivot, CalcDesiredPosition(playerPos));

	// 補間の途中状態を捨てて、この位置・注視点をそのまま採用させる
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
	Vector3 forward = Normalize(player_->GetWorldTransform()->GetForward());
	float lookT = 1.0f - std::exp(-lookLagSpeed_ * DeltaTime::GetDeltaTime());
	smoothedLookOffset_ = Lerp(smoothedLookOffset_, forward * lookForwardOffset_, lookT);

	Vector3 lookTarget = playerPos + Vector3(0, lookHeight_, 0) + smoothedLookOffset_ - Vector3(0, sin(pitch_) * lookPitchDip_, 0);

	float t = 1.0f - std::exp(-positionLagSpeed_ * DeltaTime::GetDeltaTime());
	GetTranslate() = Lerp(GetTranslate(), desiredPos, t);

	// 壁にめり込まないよう、プレイヤーとの間に遮蔽物があれば手前へ引き寄せる
	GetTranslate() = ResolveCameraCollision(playerPos + Vector3(0.0f, lookHeight_, 0.0f), GetTranslate());

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

	float distance = std::clamp(distToEnemy * lockOnDistanceMul_, lockOnDistanceMin_, lockOnDistanceMax_);
	// ⑨ 攻撃中はロックオンでも少し寄る
	distance *= actionZoomScale_;

	const float height = lockOnHeight_;

	Vector3 right = Normalize(Cross(Vector3(0, 1, 0), toEnemy));

	Vector3 cameraPos = playerPos - toEnemy * distance + right * lockOnRightOffset_;

	cameraPos.y += height;

	Vector3 lookTarget = (playerPos + enemyPos) * 0.5f + Vector3(0, lookHeight_, 0);

	float t = 1.0f - std::exp(-lockOnLagSpeed_ * DeltaTime::GetDeltaTime());
	GetTranslate() = Lerp(GetTranslate(), cameraPos, t);

	// 壁にめり込まないよう、プレイヤーとの間に遮蔽物があれば手前へ引き寄せる
	GetTranslate() = ResolveCameraCollision(playerPos + Vector3(0.0f, lookHeight_, 0.0f), GetTranslate());

	ApplySmoothLookAt(lookTarget);

	// Free復帰用
	yaw_ = std::atan2(toEnemy.x, toEnemy.z);
}
