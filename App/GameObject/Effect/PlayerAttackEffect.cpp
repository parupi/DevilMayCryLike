#include "PlayerAttackEffect.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Camera/GameCamera.h"
#include "Graphics/Rendering/Effect/WeaponTrail.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/PostEffect/RadialBlurEffect.h"
#include "Graphics/Rendering/PostEffect/SpeedLineEffect.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "Platform/WindowManager.h"
#include "Utility/DeltaTime.h"
#include "Utility/Logger.h"
#include "Math/MathUtils.h"
#include "Math/Vector2.h"
#include <algorithm>
#include <cmath>

namespace {
	// 納刀モーション。攻撃と同じ仕組みで流れてくるが、攻撃ではないので演出を出さない
	constexpr const char* kSheatheName = "Sheathe";

	// ── VFX（Resource/VFX/*.vfx.json）──
	constexpr const char* kHitSlashVfx = "PlayerHitSlash";
	constexpr const char* kHitHeavyVfx = "PlayerHitHeavy";
	constexpr const char* kSwingSparkVfx = "PlayerSwingSpark";
	constexpr const char* kBladeMoteVfx = "PlayerBladeMote";
	constexpr const char* kBladeMoteGoldVfx = "PlayerBladeMoteGold";
	constexpr const char* kAirSlashVfx = "PlayerAirSlash";
	constexpr const char* kThrustRingVfx = "PlayerThrustRing";
	constexpr const char* kLaunchRiseVfx = "PlayerLaunchRise";
	// ボスの叩きつけ用の地面の演出を、大きさの倍率で小さくして使い回す
	constexpr const char* kGroundImpactVfx = "BossImpact";
	constexpr const char* kGroundCrackVfx = "BossGroundCrack";
	constexpr const char* kGroundDustVfx = "BossDust";
	// 回避の砂埃（GameScene が読む）
	constexpr const char* kFootDustVfx = "DodgeDust";
	// 叩きつけで地面を打った火花（鎧に当てたときの熱い火花と同じ）
	constexpr const char* kSlamSparkVfx = "HitMatArmor";

	// ── 画面効果（シーンをまたいで残る）──
	constexpr const char* kRadialBlurName = "PlayerAttackRadialBlur";
	constexpr const char* kSpeedLineName = "PlayerThrustSpeedLine";

	// ── 軌跡 ──
	constexpr uint32_t kTrailSubdivisions = 4;
	constexpr float kCoreWidthRatio = 0.28f;   // 刃先の細い帯の幅（刃の長さに対する割合）
	constexpr float kCoreLifetimeScale = 0.7f; // 細い帯は太い帯より先に消える
	constexpr float kSmearMinTipSpeed = 7.0f;  // 刃先がこれより速いときだけブレを出す[m/s]
	const Vector4 kSmearColor{ 0.88f, 0.90f, 0.96f, 0.22f };

	// ── 刀身の発光 ──
	constexpr float kGlowRiseRate = 30.0f;     // 光り始める速さ
	constexpr float kStartupGlowScale = 0.6f;  // 構えきった時点の光（振り始めの光に対する割合）
	constexpr float kSwingFlashScale = 1.25f;  // 振り始めの一瞬
	constexpr float kActiveGlowScale = 0.7f;   // 振っている間

	// ── 構え・溜め ──
	constexpr float kStartupMoteIntervalLight = 0.06f;
	constexpr float kStartupMoteIntervalHeavy = 0.035f;
	constexpr float kChargeMoteIntervalStart = 0.07f;
	constexpr float kChargeMoteIntervalEnd = 0.025f;
	constexpr float kChargeFullShake = 0.12f;
	constexpr float kChargeFullRingScale = 1.2f;

	// ── 突進 ──
	constexpr float kThrustExtraInterval = 0.05f;
	constexpr float kThrustFovPunch = 0.06f;
	constexpr float kThrustSpeedLineStrength = 0.4f;
	constexpr float kThrustSpeedLineTime = 0.35f;
	constexpr float kThrustBlurStrength = 0.02f;

	// ── 打ち上げ ──
	constexpr float kLaunchImpactSize = 0.2f;
	constexpr float kLaunchBlurStrength = 0.015f;

	// ── 強攻撃 ──
	constexpr float kHeavyBlurStrength = 0.018f;

	// ── 叩きつけ ──
	constexpr float kSlamGroundHeight = 0.35f; // 刃先が足元からこの高さまで下りたら地面に届いた
	constexpr float kSlamEarliest = 0.25f;     // 振りのこの割合より前は届いたとみなさない
	constexpr float kSlamLatest = 0.85f;       // 届かなくてもこの割合で出す

	constexpr float kBlurDuration = 0.14f;

	const Vector3 kUp{ 0.0f, 1.0f, 0.0f };

	Vector3 SafeNormalize(const Vector3& v, const Vector3& fallback) {
		const float length = Length(v);
		return (length > 0.0001f) ? v * (1.0f / length) : fallback;
	}

	float Random01(std::mt19937& rng) {
		return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
	}

	// ワールド座標を画面のUVへ。カメラの後ろ・画面外なら画面中央
	Vector2 ToScreenUV(const Vector3& worldPosition) {
		const Vector2 kCenter{ 0.5f, 0.5f };
		BaseCamera* camera = CameraManager::GetInstance().GetCurrentCamera();
		if (!camera || !camera->IsInView(worldPosition)) return kCenter;
		const Vector2 screen = camera->WorldToScreen(worldPosition,
			static_cast<int>(WindowManager::kGameWidth), static_cast<int>(WindowManager::kGameHeight));
		return {
			screen.x / static_cast<float>(WindowManager::kGameWidth),
			screen.y / static_cast<float>(WindowManager::kGameHeight) };
	}
}

// ─────────────────────────────────────────
// 見た目の種類
// ─────────────────────────────────────────

const PlayerAttackEffect::StyleProfile& PlayerAttackEffect::GetProfile(AttackVfxStyle style) {
	// 通常は白＋青、強い技は白＋金（PlayerAttackVFX.md の色使い。ジャスト回避の青とも揃えている）
	//                                  太い帯の色                        刃先の帯の色                     刀身の光の色              光  抜け  帯    ブレ   光の粒             初速の火花
	static const StyleProfile kSlash { { 0.30f, 0.62f, 1.00f, 0.60f }, { 1.00f, 1.00f, 1.00f, 0.95f }, { 0.55f, 0.80f, 1.00f }, 0.9f, 14.0f, 0.14f, 0.05f, kBladeMoteVfx,     0.6f };
	static const StyleProfile kHeavy { { 1.00f, 0.72f, 0.28f, 0.62f }, { 1.00f, 0.97f, 0.88f, 1.00f }, { 1.00f, 0.78f, 0.40f }, 1.6f,  6.0f, 0.20f, 0.10f, kBladeMoteGoldVfx, 1.2f };
	static const StyleProfile kThrust{ { 0.45f, 0.80f, 1.00f, 0.60f }, { 1.00f, 1.00f, 1.00f, 0.95f }, { 0.60f, 0.85f, 1.00f }, 1.3f,  9.0f, 0.12f, 0.08f, kBladeMoteVfx,     1.0f };
	static const StyleProfile kLaunch{ { 0.35f, 0.70f, 1.00f, 0.60f }, { 1.00f, 1.00f, 1.00f, 0.95f }, { 0.60f, 0.85f, 1.00f }, 1.2f,  9.0f, 0.28f, 0.08f, kBladeMoteVfx,     0.8f };
	static const StyleProfile kSlam  { { 1.00f, 0.66f, 0.22f, 0.65f }, { 1.00f, 0.95f, 0.85f, 1.00f }, { 1.00f, 0.72f, 0.30f }, 1.4f,  5.0f, 0.24f, 0.12f, kBladeMoteGoldVfx, 1.4f };

	switch (style) {
	case AttackVfxStyle::Heavy:  return kHeavy;
	case AttackVfxStyle::Thrust: return kThrust;
	case AttackVfxStyle::Launch: return kLaunch;
	case AttackVfxStyle::Slam:   return kSlam;
	default:                     return kSlash;
	}
}

AttackVfxStyle PlayerAttackEffect::ResolveStyle(const std::string& attackName, const AttackData& data) {
	if (data.vfxStyle != AttackVfxStyle::Auto) return data.vfxStyle;

	// 溜めて振る攻撃は叩きつけ
	if (data.isCharge) return AttackVfxStyle::Slam;
	// スティンガーは突進（吹き飛ばしの攻撃でもあるので、先に名前で見る）
	if (attackName == "Stinger") return AttackVfxStyle::Thrust;
	if (data.knockback.type == ReactionType::Launch) return AttackVfxStyle::Launch;
	// コンボの締め・カウンターなど、大きく吹き飛ばす攻撃は強攻撃
	if (data.knockback.type == ReactionType::Knockback) return AttackVfxStyle::Heavy;
	// 前へ大きく踏み込む攻撃は突進として見せる
	if (Length(data.moveVelocity) >= 15.0f) return AttackVfxStyle::Thrust;
	return AttackVfxStyle::Slash;
}

void PlayerAttackEffect::LoadVfx() {
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む
	static constexpr const char* kNames[] = {
		kHitSlashVfx, kHitHeavyVfx, kSwingSparkVfx, kBladeMoteVfx, kBladeMoteGoldVfx,
		kAirSlashVfx, kThrustRingVfx, kLaunchRiseVfx,
		// 当たった相手の材質ごとの破片（PlayerWeapon が名前で呼ぶ）
		"HitMatFlesh", "HitMatBone", "HitMatArmor", "HitMatWood",
		kGroundImpactVfx, kGroundCrackVfx, kGroundDustVfx,
	};
	auto& particles = ParticleManager::GetInstance();
	for (const char* name : kNames) {
		if (particles.GetEmitters().contains(name)) continue;
		if (!particles.LoadVFX(name)) {
			Logger::Log(std::string("PlayerAttackEffect: Resource/VFX/") + name + ".vfx.json を読み込めませんでした\n");
		}
	}
}

// ─────────────────────────────────────────
// 生成・破棄
// ─────────────────────────────────────────

PlayerAttackEffect::PlayerAttackEffect() = default;

PlayerAttackEffect::~PlayerAttackEffect() {
	// 画面効果はシーンをまたいで残るので、焼き付けたまま消えないよう必ず切る。
	// 刀身のレンダラーはシーンと一緒に先に消えていることがあるので触らない
	if (radialBlur_) {
		radialBlur_->GetEffectData().strength = 0.0f;
		radialBlur_->SetActive(false);
	}
	if (speedLine_) {
		speedLine_->GetEffectData().strength = 0.0f;
		speedLine_->SetActive(false);
	}
}

void PlayerAttackEffect::Initialize(Player* player) {
	player_ = player;
	rng_.seed(std::random_device{}());

	auto makeTrail = [](bool additive) {
		auto trail = std::make_unique<WeaponTrail>();
		trail->Initialize();
		trail->SetSubdivisions(kTrailSubdivisions);
		trail->SetAdditive(additive);
		return trail;
	};
	// 太い帯は半透明（明るい床の上でも色が残る）、刃先の帯は加算（光って見える）、ブレは半透明
	mainTrail_ = makeTrail(false);
	coreTrail_ = makeTrail(true);
	smearTrail_ = makeTrail(false);
	smearTrail_->SetTintColor(kSmearColor);

	if (player_ && player_->GetWeapon()) {
		bladeRenderer_ = player_->GetWeapon()->GetRenderer("PlayerWeapon");
	}

	OffScreenManager& offScreen = OffScreenManager::GetInstance();
	if (!offScreen.FindEffect(kRadialBlurName)) {
		auto blur = std::make_unique<RadialBlurEffect>(kRadialBlurName);
		blur->SetActive(false);
		offScreen.AddEffect(std::move(blur));
	}
	if (!offScreen.FindEffect(kSpeedLineName)) {
		auto lines = std::make_unique<SpeedLineEffect>(kSpeedLineName);
		lines->SetActive(false);
		offScreen.AddEffect(std::move(lines));
	}
	radialBlur_ = dynamic_cast<RadialBlurEffect*>(offScreen.FindEffect(kRadialBlurName));
	speedLine_ = dynamic_cast<SpeedLineEffect*>(offScreen.FindEffect(kSpeedLineName));
	if (radialBlur_) { radialBlur_->GetEffectData().strength = 0.0f; radialBlur_->SetActive(false); }
	if (speedLine_) { speedLine_->GetEffectData().strength = 0.0f; speedLine_->SetActive(false); }

	style_ = AttackVfxStyle::Slash;
	profile_ = &GetProfile(style_);
	ApplyProfile();
}

void PlayerAttackEffect::ApplyProfile() {
	mainTrail_->SetTintColor(profile_->outerColor);
	mainTrail_->SetLifetime(profile_->trailLifetime);
	coreTrail_->SetTintColor(profile_->coreColor);
	coreTrail_->SetLifetime(profile_->trailLifetime * kCoreLifetimeScale);
	smearTrail_->SetLifetime(profile_->smearLifetime);
}

// ─────────────────────────────────────────
// 更新
// ─────────────────────────────────────────

void PlayerAttackEffect::Update(float deltaTime) {
	if (!player_ || !profile_) return;
	pulseTime_ += deltaTime;

	UpdateBladePose(deltaTime);

	PlayerCombat* combat = player_->GetCombat();
	const PlayerStateAttack* attack = combat ? combat->GetCurrentAttack() : nullptr;
	if (attack && attack->GetAttackName() == kSheatheName) {
		attack = nullptr;
	}

	Stage stage = Stage::None;
	if (attack) {
		if (attack->IsCharging()) {
			stage = Stage::Charge;
		} else if (attack->IsStartupPhase()) {
			stage = Stage::Startup;
		} else if (attack->IsActivePhase()) {
			stage = Stage::Active;
		} else {
			stage = Stage::After;
		}

		// 新しい攻撃の始まり（別の技へ派生した／同じ技を出し直した）
		const bool restarted = (stage == Stage::Startup || stage == Stage::Charge)
			&& (prevStage_ == Stage::None || prevStage_ == Stage::Active || prevStage_ == Stage::After);
		if (attack->GetAttackName() != attackName_ || restarted) {
			OnAttackBegin(*attack);
			prevStage_ = Stage::None;
		}
	} else {
		attackName_.clear();
	}

	switch (stage) {
	case Stage::Startup:
		UpdateStartup(*attack, deltaTime);
		break;
	case Stage::Charge:
		UpdateCharge(*attack, deltaTime);
		break;
	case Stage::Active:
		chargeRatio_ = attack->GetChargeProgress();
		if (prevStage_ != Stage::Active) {
			OnSwingBegin();
		}
		UpdateSwing(*attack, deltaTime);
		break;
	case Stage::After:
	case Stage::None:
		// 振り終わり: 軌跡は残光として寿命で消え、刀身の光は種類ごとの速さで抜けていく（オーラ消失）
		glowTarget_ = 0.0f;
		break;
	}
	prevStage_ = stage;

	mainTrail_->Update(deltaTime);
	coreTrail_->Update(deltaTime);
	smearTrail_->Update(deltaTime);

	UpdateGlow(deltaTime);
	// 画面効果はヒットストップで止めない（止めると画面がブレたまま固まる）
	UpdateScreenEffects(DeltaTime::GetDeltaTime());
}

void PlayerAttackEffect::UpdateBladePose(float deltaTime) {
	PlayerWeapon* weapon = player_->GetWeapon();
	if (!weapon) return;

	bladeTip_ = weapon->GetBladeTipWorld();
	bladeBase_ = weapon->GetBladeBaseWorld();

	// ヒットストップで時間が止まっている間は、直前の速度を保つ（当たった瞬間の火花の向きに使うため）
	if (hasPrevTip_ && deltaTime > 1.0e-4f) {
		tipVelocity_ = (bladeTip_ - prevTip_) * (1.0f / deltaTime);
	}
	prevTip_ = bladeTip_;
	hasPrevTip_ = true;
}

void PlayerAttackEffect::OnAttackBegin(const PlayerStateAttack& attack) {
	attackName_ = attack.GetAttackName();
	style_ = ResolveStyle(attackName_, attack.GetBaseAttackData());
	profile_ = &GetProfile(style_);
	ApplyProfile();

	chargeRatio_ = 0.0f;
	chargeFull_ = false;
	moteTimer_ = 0.0f;
}

void PlayerAttackEffect::UpdateStartup(const PlayerStateAttack& attack, float deltaTime) {
	// 構えるにつれて刀身が光り始める
	glowTarget_ = profile_->glowPeak * kStartupGlowScale * attack.GetStartupProgress();

	// 柄から刃先へ光の粒が流れる（強い技ほど密に）
	moteTimer_ -= deltaTime;
	if (moteTimer_ <= 0.0f) {
		moteTimer_ = (style_ == AttackVfxStyle::Slash) ? kStartupMoteIntervalLight : kStartupMoteIntervalHeavy;
		EmitMote(1.0f);
	}
}

void PlayerAttackEffect::UpdateCharge(const PlayerStateAttack& attack, float deltaTime) {
	chargeRatio_ = attack.GetChargeProgress();

	// 溜めるほど光も粒も強くなる
	glowTarget_ = profile_->glowPeak * (0.45f + 0.9f * chargeRatio_);
	moteTimer_ -= deltaTime;
	if (moteTimer_ <= 0.0f) {
		moteTimer_ = kChargeMoteIntervalStart + (kChargeMoteIntervalEnd - kChargeMoteIntervalStart) * chargeRatio_;
		EmitMote(1.0f + chargeRatio_);
	}

	// 溜めきった合図: 刀身が強く光り、足元に光の輪が広がる
	if (!chargeFull_ && chargeRatio_ >= 1.0f) {
		chargeFull_ = true;
		glow_ = profile_->glowPeak * 2.0f;
		for (int i = 0; i < 4; ++i) {
			EmitMote(1.0f);
		}
		ParticleManager::GetInstance().PlayVFX(kThrustRingVfx, GetFeet() + kUp * 0.05f, kUp, 1.0f, kChargeFullRingScale);
		AddCameraShake(kChargeFullShake);
	}
	// 溜めきった後は脈打たせて「いつ離してもいい」を見せる
	if (chargeFull_) {
		glowTarget_ *= 0.85f + 0.3f * (0.5f + 0.5f * std::sin(pulseTime_ * 14.0f));
	}
}

void PlayerAttackEffect::OnSwingBegin() {
	auto& particles = ParticleManager::GetInstance();
	slamImpactDone_ = false;
	thrustTimer_ = 0.0f;

	// 刀身が一瞬強く光る（溜めたぶん強く）
	glow_ = (std::max)(glow_, profile_->glowPeak * kSwingFlashScale * (1.0f + chargeRatio_ * 0.5f));

	// 前の振りのブレとつながらないよう切っておく（太い帯はコンボの流れとして残す）
	smearTrail_->Clear();

	// 初速の火花（柄の近くから、振り出す向きへ）
	const Vector3 bladeDirection = SafeNormalize(bladeTip_ - bladeBase_, kUp);
	particles.PlayVFX(kSwingSparkVfx, bladeBase_ + (bladeTip_ - bladeBase_) * 0.15f,
		SafeNormalize(tipVelocity_, bladeDirection), profile_->swingSparkCount);

	const Vector3 forward = GetForward();
	switch (style_) {
	case AttackVfxStyle::Thrust:
		// 前方へ衝撃リング・足元の砂埃・集中線・画角の蹴り
		particles.PlayVFX(kThrustRingVfx, GetChest() + forward * 0.6f, forward, 1.0f);
		particles.PlayVFX(kFootDustVfx, GetFeet(), SafeNormalize(forward * -1.0f + kUp * 0.3f, kUp), 1.5f);
		AddCameraFovPunch(kThrustFovPunch);
		speedLineTimer_ = kThrustSpeedLineTime;
		speedLineDuration_ = kThrustSpeedLineTime;
		speedLineStrength_ = kThrustSpeedLineStrength;
		PlayRadialBlur(GetChest(), kThrustBlurStrength, kBlurDuration);
		break;

	case AttackVfxStyle::Launch:
		// 足元の小さな衝撃と、上へ昇る光
		particles.PlayVFX(kGroundImpactVfx, GetFeet() + kUp * 0.05f, kUp, 0.5f, kLaunchImpactSize);
		particles.PlayVFX(kLaunchRiseVfx, GetFeet(), kUp, 1.0f);
		PlayRadialBlur(GetChest(), kLaunchBlurStrength, kBlurDuration);
		break;

	case AttackVfxStyle::Heavy:
		// 前方へ風の筋
		particles.PlayVFX(kAirSlashVfx, GetChest() + forward * 0.8f, forward, 0.6f);
		PlayRadialBlur(GetChest(), kHeavyBlurStrength, kBlurDuration);
		break;

	case AttackVfxStyle::Slam:
		// 地面の衝撃は刃先が届いた瞬間（UpdateSwing）
		PlayRadialBlur(GetChest(), 0.02f + 0.015f * chargeRatio_, kBlurDuration);
		break;

	default:
		break;
	}
}

void PlayerAttackEffect::UpdateSwing(const PlayerStateAttack& attack, float deltaTime) {
	// 二重の軌跡: 刃全体の太い色の帯と、刃先寄りの細い白い帯
	mainTrail_->AddPoint(bladeTip_, bladeBase_);
	coreTrail_->AddPoint(bladeTip_, bladeTip_ + (bladeBase_ - bladeTip_) * kCoreWidthRatio);
	// 刃のブレ（モーションブラーの代わり）。速く振っている間だけ出るので、振り出しで現れて振り抜きで消える
	if (Length(tipVelocity_) >= kSmearMinTipSpeed) {
		smearTrail_->AddPoint(bladeTip_, bladeBase_);
	}

	glowTarget_ = profile_->glowPeak * kActiveGlowScale;

	switch (style_) {
	case AttackVfxStyle::Thrust: {
		// 突っ込んでいる間、前方の風の筋と足元の砂埃を出し続ける
		thrustTimer_ -= deltaTime;
		if (thrustTimer_ <= 0.0f) {
			thrustTimer_ = kThrustExtraInterval;
			const Vector3 forward = GetForward();
			auto& particles = ParticleManager::GetInstance();
			particles.PlayVFX(kAirSlashVfx, GetChest() + forward * 1.2f, forward, 0.6f);
			particles.PlayVFX(kFootDustVfx, GetFeet(), SafeNormalize(forward * -1.0f + kUp * 0.2f, kUp), 0.5f);
		}
		break;
	}
	case AttackVfxStyle::Slam: {
		const float progress = attack.GetActiveProgress();
		const bool reachedGround = bladeTip_.y <= GetFeet().y + kSlamGroundHeight;
		if (!slamImpactDone_ && progress >= kSlamEarliest && (reachedGround || progress >= kSlamLatest)) {
			slamImpactDone_ = true;
			PlaySlamImpact();
		}
		break;
	}
	default:
		break;
	}
}

void PlayerAttackEffect::PlaySlamImpact() {
	auto& particles = ParticleManager::GetInstance();
	const float power = chargeRatio_;
	// 刃先の真下の地面
	const Vector3 point{ bladeTip_.x, GetFeet().y, bladeTip_.z };

	// ボスの叩きつけ用の演出を人の大きさまで縮めて使う。
	// 0.45倍でも画面の半分近くが白い輪と土煙で埋まったので、さらに小さくしてある
	particles.PlayVFX(kGroundImpactVfx, point + kUp * 0.05f, kUp, 0.6f + 0.4f * power, 0.25f + 0.15f * power);
	particles.PlayVFX(kGroundCrackVfx, point, kUp, 1.0f, 0.35f + 0.15f * power);
	particles.PlayVFX(kGroundDustVfx, point, kUp, 0.35f + 0.35f * power, 0.35f + 0.15f * power);
	// 地面を打った火花（熱い金属の火花を使う。強攻撃の光まで重ねると真っ白になる）
	particles.PlayVFX(kSlamSparkVfx, point + kUp * 0.1f, kUp, 1.0f + power);

	AddCameraShake(0.25f + 0.3f * power);
	AddCameraFovPunch(-0.03f - 0.02f * power);
	PlayRadialBlur(point, 0.03f + 0.02f * power, 0.18f);
}

void PlayerAttackEffect::UpdateGlow(float deltaTime) {
	const float rate = (glowTarget_ > glow_) ? kGlowRiseRate : profile_->glowFadeRate;
	glow_ += (glowTarget_ - glow_) * (1.0f - std::exp(-rate * deltaTime));
	if (glow_ < 0.001f) {
		glow_ = 0.0f;
	}
	if (bladeRenderer_) {
		// 被弾フラッシュ（HitFlashComponent）はこの後に走って上書きするので、毎フレーム書いてよい
		bladeRenderer_->SetEmissiveTint({ profile_->glowColor.x, profile_->glowColor.y, profile_->glowColor.z, glow_ });
	}
}

void PlayerAttackEffect::UpdateScreenEffects(float realDeltaTime) {
	if (radialBlur_) {
		if (blurTimer_ > 0.0f) {
			blurTimer_ -= realDeltaTime;
			const float t = (blurDuration_ > 0.0f) ? std::clamp(blurTimer_ / blurDuration_, 0.0f, 1.0f) : 0.0f;
			radialBlur_->GetEffectData().strength = blurStrength_ * t * t;
			radialBlur_->SetActive(t > 0.0f);
		} else if (radialBlur_->IsActive()) {
			radialBlur_->GetEffectData().strength = 0.0f;
			radialBlur_->SetActive(false);
		}
	}

	if (speedLine_) {
		if (speedLineTimer_ > 0.0f) {
			speedLineTimer_ -= realDeltaTime;
			const float t = (speedLineDuration_ > 0.0f) ? std::clamp(speedLineTimer_ / speedLineDuration_, 0.0f, 1.0f) : 0.0f;
			speedLine_->GetEffectData().strength = speedLineStrength_ * t;
			speedLine_->SetActive(t > 0.0f);
		} else if (speedLine_->IsActive()) {
			speedLine_->GetEffectData().strength = 0.0f;
			speedLine_->SetActive(false);
		}
	}
}

// ─────────────────────────────────────────
// 描画・停止
// ─────────────────────────────────────────

void PlayerAttackEffect::Draw() {
	// ブレ → 太い帯 → 刃先の光る帯 の順に重ねる
	if (smearTrail_) smearTrail_->Draw();
	if (mainTrail_) mainTrail_->Draw();
	if (coreTrail_) coreTrail_->Draw();
}

void PlayerAttackEffect::Stop() {
	if (mainTrail_) mainTrail_->Clear();
	if (coreTrail_) coreTrail_->Clear();
	if (smearTrail_) smearTrail_->Clear();

	glow_ = 0.0f;
	glowTarget_ = 0.0f;
	if (bladeRenderer_) {
		bladeRenderer_->SetEmissiveTint({ 0.0f, 0.0f, 0.0f, 0.0f });
	}

	blurTimer_ = 0.0f;
	speedLineTimer_ = 0.0f;
	if (radialBlur_) { radialBlur_->GetEffectData().strength = 0.0f; radialBlur_->SetActive(false); }
	if (speedLine_) { speedLine_->GetEffectData().strength = 0.0f; speedLine_->SetActive(false); }
}

// ─────────────────────────────────────────
// 小物
// ─────────────────────────────────────────

void PlayerAttackEffect::EmitMote(float countScale) {
	const Vector3 bladeDirection = SafeNormalize(bladeTip_ - bladeBase_, kUp);
	ParticleManager::GetInstance().PlayVFX(profile_->moteVfx, RandomOnBlade(), bladeDirection, countScale);
}

void PlayerAttackEffect::PlayRadialBlur(const Vector3& worldCenter, float strength, float duration) {
	if (!radialBlur_ || duration <= 0.0f) return;
	// 再生中のより強いブラーは潰さない
	if (blurTimer_ > 0.0f && blurDuration_ > 0.0f) {
		const float remaining = blurTimer_ / blurDuration_;
		if (strength < blurStrength_ * remaining * remaining) return;
	}
	radialBlur_->GetEffectData().center = ToScreenUV(worldCenter);
	blurStrength_ = strength;
	blurDuration_ = duration;
	blurTimer_ = duration;
}

void PlayerAttackEffect::AddCameraShake(float trauma) const {
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddShake(trauma);
	}
}

void PlayerAttackEffect::AddCameraFovPunch(float add) const {
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddFovPunch(add);
	}
}

Vector3 PlayerAttackEffect::RandomOnBlade() {
	// 柄に近すぎる所は避ける
	return bladeBase_ + (bladeTip_ - bladeBase_) * (0.15f + 0.85f * Random01(rng_));
}

Vector3 PlayerAttackEffect::GetFeet() const {
	return player_->GetWorldTransform()->GetTranslation() + Vector3{ 0.0f, Player::kModelOffsetY, 0.0f };
}

Vector3 PlayerAttackEffect::GetChest() const {
	return player_->GetWorldTransform()->GetTranslation() + Vector3{ 0.0f, 0.15f, 0.0f };
}

Vector3 PlayerAttackEffect::GetForward() const {
	// プレイヤーの正面はローカル +Z（敵とは逆）
	const Vector3 forward = RotateVector({ 0.0f, 0.0f, 1.0f }, player_->GetWorldTransform()->GetRotation());
	return SafeNormalize(Vector3{ forward.x, 0.0f, forward.z }, Vector3{ 0.0f, 0.0f, 1.0f });
}
