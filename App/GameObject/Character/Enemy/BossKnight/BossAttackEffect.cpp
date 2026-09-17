#include "BossAttackEffect.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include "BossVfxUtil.h"
#include "State/BossStateRoar.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/Component/EnemyBoneAttackComponent.h"
#include "GameObject/Character/Player/Player.h"
#include "Graphics/Rendering/Effect/WeaponTrail.h"
#include "Audio/GameSoundLibrary.h"
#include "Audio/SoundManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/PostEffect/HitFlashEffect.h"
#include "Graphics/Rendering/PostEffect/RadialBlurEffect.h"
#include "Graphics/Rendering/PostEffect/SpeedLineEffect.h"
#include "Graphics/Rendering/PostEffect/HeatDistortionEffect.h"
#include "Utility/Logger.h"

namespace {
	// ── ジョイント（Dragon.gltf）──
	constexpr const char* kJointNose = "Nose";
	constexpr const char* kJointHead = "Head";
	constexpr const char* kJointNeck = "Neck";
	constexpr const char* kJointFootLeft = "Feet.L";
	constexpr const char* kJointFootRight = "Feet.R";
	constexpr const char* kJointWingTipLeft = "Wing4.L";
	constexpr const char* kJointWingTipRight = "Wing4.R";
	constexpr const char* kJointShoulderLeft = "Shoulder.L";
	constexpr const char* kJointShoulderRight = "Shoulder.R";

	// 位置の目安（スケール1基準。配置スケールを掛ける）
	constexpr float kBodyCenterHeight = 0.9f;  // 体の中心の高さ（集中線・ブラーの中心）
	constexpr float kFootSideOffset = 0.45f;   // ジョイントが無いときの足の左右の間隔

	// ── 噛みつき（溜め約1.0秒・判定約0.2秒）──
	constexpr float kBiteStanceDust = 0.6f;     // 踏ん張った瞬間の足元の土煙の量
	constexpr float kBiteScrapeInterval = 0.2f; // 溜めの間、爪が地面を擦る間隔
	constexpr float kBiteStrikeShake = 0.12f;
	constexpr float kBiteBlurStrength = 0.03f;  // 振り抜きのモーションブラー
	constexpr float kBiteBlurDuration = 0.14f;
	constexpr float kBiteMissImpact = 0.4f;     // 顎の下の地面の衝撃の量（リングは出ない量）
	constexpr float kBiteTrailLifetime = 0.18f;

	// ── 叩きつけ（溜め約1.8秒・判定約0.28秒）──
	constexpr float kSlamRadius = 2.4f;          // 予兆の円の半径（スケール1基準）
	constexpr float kSlamGatherStart = 0.6f;     // 溜めのこの割合から「力を溜める」段階
	constexpr float kSlamDustInterval = 0.24f;   // 前半の砂の舞い上がり
	constexpr float kSlamGatherInterval = 0.12f; // 後半は間隔を詰める
	constexpr float kSlamRumble = 0.10f;         // 1秒あたりの揺れ（前半）
	constexpr float kSlamGatherRumble = 0.28f;   // 1秒あたりの揺れ（後半）
	constexpr float kSlamImpact = 2.2f;          // 叩きつけの衝撃の量
	constexpr float kSlamShake = 0.65f;
	constexpr float kSlamBlurStrength = 0.04f;
	constexpr float kSlamBlurDuration = 0.18f;
	constexpr int   kSlamSpreadDustCount = 6;    // 余韻で広がる土煙の方向の数

	// ── 突進（溜め約0.9秒・判定約0.44秒）──
	constexpr float kRushPlantStart = 0.5f;      // 溜めのこの割合から踏み込み（後ろへ土煙）
	constexpr float kRushCrackAt = 0.45f;        // 地面のひびを入れる溜めの割合
	constexpr float kRushScrapeInterval = 0.18f;
	constexpr float kRushPlantInterval = 0.09f;
	constexpr float kRushPlantRumble = 0.2f;
	constexpr float kRushDustInterval = 0.06f;   // 突っ込んでいる間の足元の土煙
	constexpr float kRushRumble = 0.4f;
	constexpr float kRushFovPunch = 0.08f;
	constexpr float kRushStartShake = 0.25f;
	constexpr float kRushStopImpact = 1.5f;
	constexpr float kRushStopShake = 0.45f;
	constexpr float kRushStopForward = 1.2f;     // 止まった位置の少し前で土煙を上げる（スケール1基準）
	constexpr float kRushTrailLifetime = 0.3f;
	constexpr float kSpeedLineStrength = 0.55f;
	constexpr float kSpeedLineFollowRate = 10.0f;

	// ── 咆哮 ──
	constexpr float kRoarFovPunch = 0.06f;            // 声に押し返される
	constexpr float kRoarDistortionDuration = 0.6f;
	constexpr float kRoarDistortionStrength = 0.012f;
	constexpr float kRoarDistortionRadiusStart = 0.5f;// スケール1基準
	constexpr float kRoarDistortionRadiusEnd = 4.0f;

	// ── 被弾の閃光 ──
	constexpr float kHitFlashDuration = 0.12f;

	// ── 移動・着地 ──
	constexpr float kMoveDustSpeed = 0.8f;      // この速さ（スケール1基準）を超えて動いている間だけ砂を巻き上げる
	constexpr float kMoveDustInterval = 0.3f;
	constexpr float kMoveDustAmount = 0.5f;
	constexpr float kLandingSpeed = 3.0f;       // この速さ以上で落ちてきた着地だけ衝撃を出す
	constexpr float kLandingImpact = 1.0f;
	constexpr float kLandingShake = 0.35f;
	// 羽ばたきの間隔[秒]。待機中もゆっくり羽ばたいている想定
	constexpr float kWingbeatInterval = 1.6f;

	// 大きく飛んだフレームで一度に大量に出さない
	constexpr float kMaxStep = 0.1f;

	const Vector3 kUp{ 0.0f, 1.0f, 0.0f };

	Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback) {
		return (Length(v) > 0.0001f) ? Normalize(v) : fallback;
	}
}

void BossAttackEffect::LoadVfx() {
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む
	static constexpr const char* kNames[] = {
		kDustVfx, kClawScrapeVfx, kImpactVfx, kGroundCrackVfx, kRoarSalivaVfx,
	};
	auto& particles = ParticleManager::GetInstance();
	for (const char* name : kNames) {
		if (particles.GetEmitters().contains(name)) continue;
		if (!particles.LoadVFX(name)) {
			Logger::Log(std::string("BossAttackEffect: Resource/VFX/") + name + ".vfx.json を読み込めませんでした\n");
		}
	}
}

BossAttackEffect::BossAttackEffect() = default;

BossAttackEffect::~BossAttackEffect() {
	// 画面効果はシーンをまたいで残るので、焼き付けたまま消えないよう必ず切る
	if (hitFlash_) {
		hitFlash_->GetEffectData().flashColor.w = 0.0f;
		hitFlash_->SetActive(false);
	}
	if (radialBlur_) {
		radialBlur_->GetEffectData().strength = 0.0f;
		radialBlur_->SetActive(false);
	}
	if (speedLine_) {
		speedLine_->GetEffectData().strength = 0.0f;
		speedLine_->SetActive(false);
	}
	if (roarDistortion_) {
		roarDistortion_->GetEffectData().strength = 0.0f;
		roarDistortion_->SetActive(false);
	}
}

void BossAttackEffect::Initialize(const std::string& ownerName) {
	ownerName_ = ownerName;
	rng_.seed(std::random_device{}());

	// ── 軌跡 ──
	biteTrail_ = std::make_unique<WeaponTrail>();
	biteTrail_->Initialize();
	biteTrail_->SetLifetime(kBiteTrailLifetime);
	biteTrail_->SetTintColor({ 1.0f, 0.95f, 0.85f, 0.75f });

	wingTrailLeft_ = std::make_unique<WeaponTrail>();
	wingTrailLeft_->Initialize();
	wingTrailLeft_->SetLifetime(kRushTrailLifetime);
	wingTrailLeft_->SetTintColor({ 0.85f, 0.9f, 1.0f, 0.55f });

	wingTrailRight_ = std::make_unique<WeaponTrail>();
	wingTrailRight_->Initialize();
	wingTrailRight_->SetLifetime(kRushTrailLifetime);
	wingTrailRight_->SetTintColor({ 0.85f, 0.9f, 1.0f, 0.55f });

	// ── 画面効果 ──
	// 同名のパスが既にあれば AddEffect 側で弾かれる（シーンをまたいで生き続けるため）。
	// 登録した順にかかるので、絵を揺らす → ぼかす → 線を足す → 閃光 の順にする
	auto& offScreen = OffScreenManager::GetInstance();
	{
		auto distortion = std::make_unique<HeatDistortionEffect>("BossRoarDistortion");
		distortion->SetActive(false);
		offScreen.AddEffect(std::move(distortion));
	}
	{
		auto blur = std::make_unique<RadialBlurEffect>("BossAttackRadialBlur");
		blur->SetActive(false);
		offScreen.AddEffect(std::move(blur));
	}
	{
		auto speedLine = std::make_unique<SpeedLineEffect>("BossRushSpeedLine");
		speedLine->SetActive(false);
		offScreen.AddEffect(std::move(speedLine));
	}
	{
		auto flash = std::make_unique<HitFlashEffect>("BossAttackHitFlash");
		flash->SetActive(false);
		offScreen.AddEffect(std::move(flash));
	}
	roarDistortion_ = dynamic_cast<HeatDistortionEffect*>(offScreen.FindEffect("BossRoarDistortion"));
	radialBlur_ = dynamic_cast<RadialBlurEffect*>(offScreen.FindEffect("BossAttackRadialBlur"));
	speedLine_ = dynamic_cast<SpeedLineEffect*>(offScreen.FindEffect("BossRushSpeedLine"));
	hitFlash_ = dynamic_cast<HitFlashEffect*>(offScreen.FindEffect("BossAttackHitFlash"));

	// 前のボスが焼き付けたまま消えていても、効果が残った状態で始まらないようにする
	if (hitFlash_) { hitFlash_->GetEffectData().flashColor.w = 0.0f; hitFlash_->SetActive(false); }
	if (radialBlur_) { radialBlur_->GetEffectData().strength = 0.0f; radialBlur_->SetActive(false); }
	if (speedLine_) { speedLine_->GetEffectData().strength = 0.0f; speedLine_->SetActive(false); }
	if (roarDistortion_) { roarDistortion_->GetEffectData().strength = 0.0f; roarDistortion_->SetActive(false); }
}

void BossAttackEffect::Update(Enemy& enemy, float deltaTime, BossActionKind action, const EnemyBoneAttackComponent& attack) {
	const float dt = std::clamp(deltaTime, 0.0f, kMaxStep);

	UpdateBody(enemy);

	if (action != action_) {
		OnActionChanged(action);
	}

	const bool isMelee = (action == BossActionKind::Bite || action == BossActionKind::Slam || action == BossActionKind::Rush);
	const bool hitActive = isMelee && attack.IsHitActive();
	const bool windingUp = isMelee && attack.IsWindingUp();
	const float windup = windingUp ? attack.GetWindupProgress() : 0.0f;

	switch (action) {
	case BossActionKind::Bite: UpdateBite(enemy, dt, windingUp); break;
	case BossActionKind::Slam: UpdateSlam(dt, windingUp, windup); break;
	case BossActionKind::Rush: UpdateRush(enemy, dt, windingUp, windup, hitActive); break;
	case BossActionKind::Roar: UpdateRoar(enemy, dt); break;
	default: break;
	}

	// 判定が出た瞬間・消えた瞬間
	if (hitActive && !prevHitActive_) {
		OnStrikeStart(enemy, action);
	} else if (!hitActive && prevHitActive_) {
		OnStrikeEnd(enemy, action);
	}
	prevHitActive_ = hitActive;

	// プレイヤーに当たった瞬間（HPが減ったフレーム）。どの判定が当てたかは Player 側しか知らないので、
	// 攻撃中にHPが減ったら「この攻撃が当たった」とみなす
	if (Player* player = enemy.GetPlayer()) {
		const int32_t hp = player->GetHp();
		if (prevPlayerHp_ >= 0 && hp < prevPlayerHp_ && (isMelee || action == BossActionKind::Breath)) {
			OnPlayerHit(action);
		}
		prevPlayerHp_ = hp;
	}

	const Vector3 velocity = enemy.GetVelocity();
	const float horizontalSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
	UpdateLocomotion(dt, action, enemy.GetOnGround(), velocity.y, horizontalSpeed);

	UpdateTrails(enemy, dt);
	UpdatePostEffects(dt);
}

void BossAttackEffect::Draw() {
	if (biteTrail_) biteTrail_->Draw();
	if (wingTrailLeft_) wingTrailLeft_->Draw();
	if (wingTrailRight_) wingTrailRight_->Draw();
}

// ─────────────────────────────────────────────────────────────
// 位置
// ─────────────────────────────────────────────────────────────

void BossAttackEffect::UpdateBody(Enemy& enemy) {
	scale_ = enemy.GetWorldTransform()->GetWorldScale().x;
	forward_ = enemy.GetForward();
	right_ = Vector3{ forward_.z, 0.0f, -forward_.x };
	foot_ = enemy.GetFootPosition();
	bodyCenter_ = foot_ + kUp * (kBodyCenterHeight * scale_);
}

Vector3 BossAttackEffect::JointOr(Enemy& enemy, const char* jointName, const Vector3& fallback) const {
	return BossVfxUtil::GetJointWorldPositionOr(enemy, ownerName_, jointName, fallback);
}

Vector3 BossAttackEffect::NextFoot(Enemy& enemy) {
	footSide_ ^= 1;
	const float side = (footSide_ == 0) ? -1.0f : 1.0f;
	const Vector3 fallback = foot_ + right_ * (kFootSideOffset * scale_ * side);
	Vector3 foot = JointOr(enemy, (footSide_ == 0) ? kJointFootLeft : kJointFootRight, fallback);
	// ジョイントは足の中にあるので、地面の高さへ下ろす（宙に浮いたドラゴンでも地面から砂が上がる）
	foot.y = foot_.y;
	return foot;
}

Vector3 BossAttackEffect::RandomAroundFoot(float radius) {
	std::uniform_real_distribution<float> angle(0.0f, 2.0f * std::numbers::pi_v<float>);
	std::uniform_real_distribution<float> distance(0.0f, 1.0f);
	const float a = angle(rng_);
	// 中心に寄りすぎないよう平方根で散らす
	const float r = std::sqrt(distance(rng_)) * radius;
	return foot_ + Vector3{ std::cos(a) * r, 0.0f, std::sin(a) * r };
}

// ─────────────────────────────────────────────────────────────
// 行動ごと
// ─────────────────────────────────────────────────────────────

void BossAttackEffect::OnActionChanged(BossActionKind next) {
	// 判定が出ている途中で別の行動へ移った（崩れなど）。衝撃は出さずに片付ける
	if (prevHitActive_) {
		biteTrailActive_ = false;
		wingTrailActive_ = false;
		speedLineTarget_ = 0.0f;
	}
	prevHitActive_ = false;
	windupStarted_ = false;
	rushCrackPlayed_ = false;
	windupEmitTimer_ = 0.0f;
	strikeEmitTimer_ = 0.0f;
	roarTimer_ = 0.0f;
	roarBurst_ = false;
	action_ = next;
}

void BossAttackEffect::UpdateBite(Enemy& enemy, float deltaTime, bool windingUp) {
	if (!windingUp) return;

	auto& particles = ParticleManager::GetInstance();

	// 踏ん張った瞬間、両足の下から土煙
	if (!windupStarted_) {
		windupStarted_ = true;
		particles.PlayVFX(kDustVfx, NextFoot(enemy), kUp, kBiteStanceDust);
		particles.PlayVFX(kDustVfx, NextFoot(enemy), kUp, kBiteStanceDust);
	}

	// 溜めの間、左右の爪が交互に地面を擦る
	windupEmitTimer_ += deltaTime;
	if (windupEmitTimer_ >= kBiteScrapeInterval) {
		windupEmitTimer_ = 0.0f;
		particles.PlayVFX(kClawScrapeVfx, NextFoot(enemy), SafeNormalize(forward_ * -1.0f + kUp * 0.4f, kUp));
	}
}

void BossAttackEffect::UpdateSlam(float deltaTime, bool windingUp, float windup) {
	if (!windingUp) return;

	// 前半は足元の砂がゆっくり舞い、後半（力を溜める）は間隔を詰めて地面を震わせる
	const bool gathering = windup >= kSlamGatherStart;
	const float interval = gathering ? kSlamGatherInterval : kSlamDustInterval;

	windupEmitTimer_ += deltaTime;
	if (windupEmitTimer_ >= interval) {
		windupEmitTimer_ = 0.0f;
		ParticleManager::GetInstance().PlayVFX(kDustVfx,
			RandomAroundFoot(kSlamRadius * scale_ * 0.6f), kUp, gathering ? 0.8f : 0.5f);
	}

	BossVfxUtil::AddCameraShake((gathering ? kSlamGatherRumble : kSlamRumble) * deltaTime);
}

void BossAttackEffect::UpdateRush(Enemy& enemy, float deltaTime, bool windingUp, float windup, bool hitActive) {
	auto& particles = ParticleManager::GetInstance();

	if (windingUp) {
		if (windup < kRushPlantStart) {
			// 姿勢を低くして地面を掴む: 爪元の砂と火花
			windupEmitTimer_ += deltaTime;
			if (windupEmitTimer_ >= kRushScrapeInterval) {
				windupEmitTimer_ = 0.0f;
				particles.PlayVFX(kClawScrapeVfx, NextFoot(enemy), SafeNormalize(forward_ * -1.0f + kUp * 0.4f, kUp));
			}
		} else {
			// 踏み込み: 後ろへ土煙と小石を蹴り出す
			windupEmitTimer_ += deltaTime;
			if (windupEmitTimer_ >= kRushPlantInterval) {
				windupEmitTimer_ = 0.0f;
				particles.PlayVFX(kDustVfx, NextFoot(enemy), SafeNormalize(forward_ * -1.0f + kUp * 0.35f, kUp), 0.8f);
			}
			BossVfxUtil::AddCameraShake(kRushPlantRumble * deltaTime);
		}

		// 地面を掴んだところにひびを入れる
		if (!rushCrackPlayed_ && windup >= kRushCrackAt) {
			rushCrackPlayed_ = true;
			particles.PlayVFX(kGroundCrackVfx, foot_, kUp, 0.5f);
		}
		return;
	}

	if (hitActive) {
		// 突っ込んでいる間: 足元から土煙を連続で残す
		strikeEmitTimer_ += deltaTime;
		while (strikeEmitTimer_ >= kRushDustInterval) {
			strikeEmitTimer_ -= kRushDustInterval;
			particles.PlayVFX(kDustVfx, NextFoot(enemy), SafeNormalize(forward_ * -1.0f + kUp * 0.25f, kUp), 0.7f);
		}
		BossVfxUtil::AddCameraShake(kRushRumble * deltaTime);
	}
}

void BossAttackEffect::UpdateRoar(Enemy& enemy, float deltaTime) {
	roarTimer_ += deltaTime;
	if (roarBurst_ || roarTimer_ < BossStateRoar::kBurstTime) return;
	roarBurst_ = true;

	// 吼えた瞬間（衝撃波リングと揺れは BossStateRoar::Burst が出している）。
	// 口から唾液を飛ばし、声に押し返されるように画角を広げ、口元の空気を歪ませる
	const Vector3 mouth = JointOr(enemy, kJointNose, bodyCenter_ + forward_ * scale_);
	ParticleManager::GetInstance().PlayVFX(kRoarSalivaVfx, mouth, SafeNormalize(forward_ + kUp * 0.25f, forward_));
	BossVfxUtil::AddCameraFovPunch(kRoarFovPunch);
	roarDistortionTimer_ = kRoarDistortionDuration;
	roarCenter_ = mouth;
}

void BossAttackEffect::OnStrikeStart(Enemy& enemy, BossActionKind action) {
	auto& particles = ParticleManager::GetInstance();

	switch (action) {
	case BossActionKind::Bite:
		// 顎の風切り: 鼻先から首へリボンを引く。振り抜きの一瞬だけぼかす
		biteTrail_->Clear();
		biteTrailActive_ = true;
		PlayRadialBlur(JointOr(enemy, kJointHead, bodyCenter_), kBiteBlurStrength, kBiteBlurDuration);
		BossVfxUtil::AddCameraShake(kBiteStrikeShake);
		break;

	case BossActionKind::Slam:
		// 着地: 衝撃波リング・大量の土煙・岩の破片・地面のひび、強めの揺れ
		particles.PlayVFX(kImpactVfx, foot_ + kUp * 0.05f, kUp, kSlamImpact);
		particles.PlayVFX(kGroundCrackVfx, foot_, kUp, 1.0f);
		BossVfxUtil::AddCameraShake(kSlamShake);
		PlayRadialBlur(foot_, kSlamBlurStrength, kSlamBlurDuration);
		enemy.FlashLight();
		break;

	case BossActionKind::Rush:
		// 加速: 集中線・翼の軌跡・画角を広げる
		wingTrailLeft_->Clear();
		wingTrailRight_->Clear();
		wingTrailActive_ = true;
		speedLineTarget_ = 1.0f;
		strikeEmitTimer_ = kRushDustInterval;
		BossVfxUtil::AddCameraFovPunch(kRushFovPunch);
		BossVfxUtil::AddCameraShake(kRushStartShake);
		break;

	default:
		break;
	}
}

void BossAttackEffect::OnStrikeEnd(Enemy& enemy, BossActionKind action) {
	auto& particles = ParticleManager::GetInstance();

	switch (action) {
	case BossActionKind::Bite: {
		biteTrailActive_ = false;
		// 噛み終わった顎の真下の地面から土煙（外れて地面を噛んだように見せる）
		Vector3 bitePoint = JointOr(enemy, kJointHead, bodyCenter_ + forward_ * (1.5f * scale_));
		bitePoint.y = foot_.y + 0.05f;
		particles.PlayVFX(kImpactVfx, bitePoint, kUp, kBiteMissImpact);
		particles.PlayVFX(kDustVfx, bitePoint, kUp, 0.6f);
		break;
	}

	case BossActionKind::Slam: {
		// 余韻: 叩いた円の縁から外へ土煙が広がる
		const float radius = kSlamRadius * scale_ * 0.7f;
		for (int i = 0; i < kSlamSpreadDustCount; ++i) {
			const float a = (static_cast<float>(i) / kSlamSpreadDustCount) * 2.0f * std::numbers::pi_v<float>;
			const Vector3 outward{ std::cos(a), 0.0f, std::sin(a) };
			particles.PlayVFX(kDustVfx, foot_ + outward * radius, SafeNormalize(outward + kUp * 0.3f, kUp), 0.7f);
		}
		break;
	}

	case BossActionKind::Rush:
		// 停止: 止まった先で大きな土煙と衝撃波、大きめの揺れ
		wingTrailActive_ = false;
		speedLineTarget_ = 0.0f;
		particles.PlayVFX(kImpactVfx, foot_ + forward_ * (kRushStopForward * scale_) + kUp * 0.05f, kUp, kRushStopImpact);
		BossVfxUtil::AddCameraShake(kRushStopShake);
		PlayRadialBlur(bodyCenter_, kBiteBlurStrength, kSlamBlurDuration);
		break;

	default:
		break;
	}
}

void BossAttackEffect::OnPlayerHit(BossActionKind action) {
	// プレイヤーの被弾演出（赤いビネット・体の光・ヒットストップ・揺れ）は Player 側が既に出している。
	// ここはボスの重い一撃らしさを足す白い閃光と、攻撃ごとの揺れの上乗せだけ
	float intensity = 0.3f;
	float shake = 0.15f;
	switch (action) {
	case BossActionKind::Bite:   intensity = 0.3f;  shake = 0.2f;  break;
	case BossActionKind::Slam:   intensity = 0.45f; shake = 0.3f;  break;
	case BossActionKind::Rush:   intensity = 0.4f;  shake = 0.3f;  break;
	case BossActionKind::Breath: intensity = 0.35f; shake = 0.15f; break;
	default: break;
	}

	hitFlashTimer_ = kHitFlashDuration;
	hitFlashIntensity_ = intensity;
	if (hitFlash_) {
		hitFlash_->GetEffectData().flashColor = { 1.0f, 1.0f, 1.0f, intensity };
		hitFlash_->SetActive(true);
	}
	BossVfxUtil::AddCameraShake(shake);
}

void BossAttackEffect::UpdateLocomotion(float deltaTime, BossActionKind action, bool onGround, float velocityY, float horizontalSpeed) {
	// 着地: ある程度の速さで落ちてきたときだけ、地面を揺らして土煙を上げる
	if (onGround && !prevOnGround_ && prevVelocityY_ < -kLandingSpeed) {
		ParticleManager::GetInstance().PlayVFX(kImpactVfx, foot_ + kUp * 0.05f, kUp, kLandingImpact);
		BossVfxUtil::AddCameraShake(kLandingShake);
		SoundManager::GetInstance().PlaySE3D(GameSound::kDragonLand, foot_, 0.9f);
	}
	prevOnGround_ = onGround;
	prevVelocityY_ = velocityY;

	// 移動: 羽ばたきの風で足元の砂が舞う（突進は自分で出すので、行動していない間だけ）
	if (action == BossActionKind::None && onGround && horizontalSpeed > kMoveDustSpeed * scale_) {
		moveDustTimer_ += deltaTime;
		if (moveDustTimer_ >= kMoveDustInterval) {
			moveDustTimer_ = 0.0f;
			ParticleManager::GetInstance().PlayVFX(kDustVfx, RandomAroundFoot(0.6f * scale_), kUp, kMoveDustAmount);
		}
	} else {
		moveDustTimer_ = 0.0f;
	}

	// 羽ばたき。攻撃中は攻撃の音でいっぱいになるので出さない。
	// 待機中も飛んでいるので、止まっていても鳴らす
	if (action == BossActionKind::None) {
		wingbeatTimer_ += deltaTime;
		if (wingbeatTimer_ >= kWingbeatInterval) {
			wingbeatTimer_ = 0.0f;
			SoundManager::GetInstance().PlaySE3D(GameSound::kDragonWingbeat, foot_, 0.45f);
		}
	} else {
		wingbeatTimer_ = 0.0f;
	}
}

void BossAttackEffect::UpdateTrails(Enemy& enemy, float deltaTime) {
	Vector3 tip{};
	Vector3 root{};

	if (biteTrailActive_
		&& BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointNose, tip)
		&& BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointNeck, root)) {
		biteTrail_->AddPoint(tip, root);
	}

	if (wingTrailActive_) {
		if (BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointWingTipLeft, tip)
			&& BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointShoulderLeft, root)) {
			wingTrailLeft_->AddPoint(tip, root);
		}
		if (BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointWingTipRight, tip)
			&& BossVfxUtil::TryGetJointWorldPosition(enemy, ownerName_, kJointShoulderRight, root)) {
			wingTrailRight_->AddPoint(tip, root);
		}
	}

	biteTrail_->Update(deltaTime);
	wingTrailLeft_->Update(deltaTime);
	wingTrailRight_->Update(deltaTime);
}

void BossAttackEffect::PlayRadialBlur(const Vector3& worldCenter, float strength, float duration) {
	// 再生中のより弱い振り抜きで、強い衝撃のブラーを潰さない
	if (radialBlurTimer_ > 0.0f && strength < radialBlurStrength_) return;
	radialBlurCenter_ = worldCenter;
	radialBlurStrength_ = strength;
	radialBlurDuration_ = duration;
	radialBlurTimer_ = duration;
}

void BossAttackEffect::UpdatePostEffects(float deltaTime) {
	// ── 被弾の白い閃光 ──
	if (hitFlash_ && hitFlashTimer_ > 0.0f) {
		hitFlashTimer_ -= deltaTime;
		const float k = std::clamp(hitFlashTimer_ / kHitFlashDuration, 0.0f, 1.0f);
		hitFlash_->GetEffectData().flashColor = { 1.0f, 1.0f, 1.0f, hitFlashIntensity_ * k * k };
		if (hitFlashTimer_ <= 0.0f) {
			hitFlashTimer_ = 0.0f;
			hitFlash_->GetEffectData().flashColor.w = 0.0f;
			hitFlash_->SetActive(false);
		}
	}

	// ── モーションブラー（振り抜き・衝撃の一瞬）──
	if (radialBlur_ && radialBlurTimer_ > 0.0f) {
		radialBlurTimer_ -= deltaTime;
		const float k = std::clamp(radialBlurTimer_ / radialBlurDuration_, 0.0f, 1.0f);
		Vector2 center{ 0.5f, 0.5f };
		BossVfxUtil::ToScreenUV(radialBlurCenter_, center);
		radialBlur_->GetEffectData().center = center;
		radialBlur_->GetEffectData().strength = radialBlurStrength_ * k * k;
		radialBlur_->SetActive(true);
		if (radialBlurTimer_ <= 0.0f) {
			radialBlurTimer_ = 0.0f;
			radialBlur_->GetEffectData().strength = 0.0f;
			radialBlur_->SetActive(false);
		}
	}

	// ── 突進の集中線 ──
	speedLineWeight_ += (speedLineTarget_ - speedLineWeight_) * (std::min)(kSpeedLineFollowRate * deltaTime, 1.0f);
	if (speedLineTarget_ <= 0.0f && speedLineWeight_ < 0.01f) {
		speedLineWeight_ = 0.0f;
	}
	if (speedLine_) {
		if (speedLineWeight_ > 0.0f) {
			// 突っ込んでくるボスへ線が集まる。画面外なら中央
			Vector2 center{ 0.5f, 0.5f };
			BossVfxUtil::ToScreenUV(bodyCenter_, center);
			speedLine_->GetEffectData().center = center;
			speedLine_->GetEffectData().strength = kSpeedLineStrength * speedLineWeight_;
			speedLine_->SetActive(true);
		} else if (speedLine_->IsActive()) {
			speedLine_->GetEffectData().strength = 0.0f;
			speedLine_->SetActive(false);
		}
	}

	// ── 咆哮の空気の歪み（口元から広がって消える）──
	if (roarDistortion_ && roarDistortionTimer_ > 0.0f) {
		roarDistortionTimer_ -= deltaTime;
		const float t = 1.0f - std::clamp(roarDistortionTimer_ / kRoarDistortionDuration, 0.0f, 1.0f);
		const float worldRadius = (kRoarDistortionRadiusStart + (kRoarDistortionRadiusEnd - kRoarDistortionRadiusStart) * t) * scale_;
		Vector2 uv{};
		float radius = 0.0f;
		if (roarDistortionTimer_ > 0.0f && BossVfxUtil::ProjectToScreen(roarCenter_, worldRadius, uv, radius)) {
			auto& data = roarDistortion_->GetEffectData();
			data.segmentStart = uv;
			data.segmentEnd = uv;
			data.radiusStart = radius;
			data.radiusEnd = radius;
			data.strength = kRoarDistortionStrength * (1.0f - t);
			roarDistortion_->SetActive(true);
		} else {
			roarDistortionTimer_ = 0.0f;
			roarDistortion_->GetEffectData().strength = 0.0f;
			roarDistortion_->SetActive(false);
		}
	}
}
