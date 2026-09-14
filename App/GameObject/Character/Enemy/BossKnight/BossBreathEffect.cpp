#include "BossBreathEffect.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Camera/GameCamera.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Graphics/Rendering/PostEffect/OffScreenManager.h"
#include "Graphics/Rendering/PostEffect/HitFlashEffect.h"
#include "Graphics/Rendering/PostEffect/ChromaticAberrationEffect.h"
#include "Graphics/Rendering/PostEffect/BloomEffect.h"
#include "Graphics/Rendering/PostEffect/HeatDistortionEffect.h"
#include "Graphics/Rendering/PostEffect/ColorGradingEffect.h"
#include "Platform/WindowManager.h"
#include "World3D/Camera/CameraManager.h"
#include "World3D/Light/LightManager.h"
#include "World3D/Light/DynamicPointLight.h"
#include "World3D/Light/DynamicSpotLight.h"
#include "World3D/Object/Renderer/BaseRenderer.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/Object/Model/Animation/Skeleton.h"
#include "Utility/Logger.h"

namespace {
	// ── 口の位置（スケール1基準。配置スケールを掛ける）──
	constexpr float kMouthForwardOffset = 0.12f;  // Nose ジョイントから少し前へ（鼻先の奥から吐かないように）
	constexpr float kFallbackMouthHeight = 0.75f; // ジョイントが無いとき: 体の原点からの高さ
	constexpr float kFallbackMouthForward = 0.6f; // ジョイントが無いとき: 体の原点から前へ

	// ── 溜め ──
	constexpr float kChargeIntervalStart = 0.12f; // 溜め始めの発生間隔[s]
	constexpr float kChargeIntervalEnd = 0.035f;  // 吐く直前の発生間隔[s]（詰めて切迫感を出す）
	constexpr float kChargeCountScaleEnd = 2.5f;  // 吐く直前の発生数の倍率
	constexpr float kChargeRumbleStart = 0.6f;    // 溜めのこの割合から低く揺らし始める
	constexpr float kChargeRumble = 0.35f;        // 1秒あたりに足す揺れ（最大）

	// ── 発射 ──
	constexpr float kIgniteFlashIntensity = 0.45f;
	constexpr float kIgniteFlashDuration = 0.14f;
	constexpr float kIgniteShake = 0.45f;
	constexpr float kIgniteFovPunch = 0.05f;
	constexpr float kIgniteLightBoost = 8.0f;
	constexpr float kIgniteLightBoostDuration = 0.3f;

	// ── 維持 ──
	constexpr float kCoreInterval = 0.03f;    // 中心炎
	constexpr float kOuterInterval = 0.05f;   // 外炎＋煙
	constexpr float kGroundInterval = 0.07f;  // 地面を舐める炎
	constexpr float kScorchInterval = 0.22f;  // 焦げ跡
	constexpr float kGroundDelay = 0.18f;     // 吐き始めてから炎が地面に届くまで[s]
	constexpr float kAimRatio = 0.8f;         // 帯の長さのどこで地面に当てるか（口から少し下向きに吐く）
	constexpr float kFireRumble = 0.55f;      // 1秒あたりに足す揺れ（減衰1.5/sと釣り合って小さく揺れ続ける）
	constexpr float kThinStart = 0.72f;       // 吐く時間のこの割合から炎が細くなり始める
	constexpr float kOuterStop = 0.92f;       // 外炎が止まりきる割合（残るのは中心炎だけ）
	constexpr float kCoreThinRate = 0.55f;    // 終わり際に中心炎を減らす割合

	// ── 余韻 ──
	constexpr float kTailDuration = 0.9f;        // 吐いた後に火の粉と煙が残る時間
	constexpr float kTailInterval = 0.07f;
	constexpr float kCancelFadeDuration = 0.25f; // 溜めの途中で中断されたときに光が消えるまで

	// ── ライト ──
	constexpr float kChargeLightIntensity = 5.0f;
	constexpr float kFireLightIntensity = 7.0f;
	constexpr float kLightRadiusCharge = 5.0f;   // スケール1基準
	constexpr float kLightRadiusFire = 9.0f;     // スケール1基準
	constexpr float kLightDecay = 0.5f;
	constexpr float kFireLightForward = 0.5f;    // 吐いている間は口より少し前に置き、地面まで照らす（スケール1基準）
	const Vector4 kChargeColorLow{ 1.0f, 0.12f, 0.04f, 1.0f };   // 溜め始め: 赤
	const Vector4 kChargeColorMid{ 1.0f, 0.45f, 0.1f, 1.0f };    // 半ば: 橙
	const Vector4 kChargeColorHigh{ 1.0f, 0.9f, 0.75f, 1.0f };   // 吐く直前: 白
	const Vector4 kFireColor{ 1.0f, 0.55f, 0.18f, 1.0f };

	// ── スポットライト（炎の進行方向）──
	constexpr float kSpotIntensityRate = 0.9f;  // 口元のライトに対する明るさの割合
	constexpr float kSpotCosAngle = 0.883f;     // 半角約28°。炎の帯の幅を照らす
	constexpr float kSpotDecay = 0.02f;         // 遠くまで届くよう減衰を弱くする
	constexpr float kSpotDistanceRate = 1.2f;   // 帯の長さに対する届く距離

	// ── 熱の歪み ──
	constexpr float kHeatChargeStrength = 0.004f; // 溜めの間の口元（吐く直前でこの値）
	constexpr float kHeatChargeRadius = 0.8f;     // 溜めの間の口元の歪みの半径（スケール1基準）
	constexpr float kHeatFireStrength = 0.008f;   // 吐いている間の帯
	constexpr float kHeatMouthRadius = 0.6f;      // 帯の口元側の半径（スケール1基準）
	constexpr float kHeatEndWidthRate = 1.4f;     // 帯の先の半径（炎の帯の半幅に対する割合）
	constexpr float kHeatEndRatio = 0.9f;         // 帯の先を帯の長さのどこに置くか
	constexpr float kHeatEndHeight = 0.5f;        // 帯の先の地面からの高さ（スケール1基準）

	// ── カラーグレーディング（暖色寄り）──
	constexpr float kGradingStrength = 0.8f;
	const Vector3 kGradingGain{ 1.08f, 0.97f, 0.85f };
	const Vector3 kGradingLift{ 0.03f, 0.01f, 0.0f };
	constexpr float kGradingSaturation = 1.1f;
	constexpr float kGradingContrast = 1.05f;

	// ── ポストエフェクト ──
	constexpr float kBloomBoost = 0.6f;        // ブルームの強さに足す量（最大）
	constexpr float kChromaStrength = 0.0025f; // 色収差（少量）
	constexpr float kChargePostWeight = 0.3f;  // 溜めの間の効き（吐く直前でこの値）
	constexpr float kPostFollowRate = 6.0f;

	// 要求がこの時間途絶えたら、ステートが止まったとみなして余韻へ移る
	constexpr float kRequestLostTimeout = 0.2f;
	// 大きく飛んだフレームで一度に大量に出さない
	constexpr float kMaxStep = 0.1f;

	float Smoothstep(float edge0, float edge1, float x) {
		const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	Vector4 LerpColor(const Vector4& a, const Vector4& b, float t) {
		return a + (b - a) * t;
	}

	const Vector3 kUp{ 0.0f, 1.0f, 0.0f };
}

void BossBreathEffect::LoadVfx() {
	// パーティクルグループはシーンをまたいで残るので、未登録のときだけ読む。
	// 毎回読むとパーティクルエディタでの調整がボスを1体置くたびに巻き戻る
	static constexpr const char* kNames[] = {
		kChargeVfx, kIgniteVfx, kCoreVfx, kOuterVfx, kGroundVfx, kScorchVfx, kTailVfx,
	};
	auto& particles = ParticleManager::GetInstance();
	for (const char* name : kNames) {
		if (particles.GetEmitters().contains(name)) continue;
		if (!particles.LoadVFX(name)) {
			Logger::Log(std::string("BossBreathEffect: Resource/VFX/") + name + ".vfx.json を読み込めませんでした\n");
		}
	}
}

BossBreathEffect::~BossBreathEffect() {
	// シーン終了時は DeleteAllLight で先に消えていることもある（その場合は何も起きない）
	LightManager::GetInstance().RemoveLight(light_);
	LightManager::GetInstance().RemoveLight(spotLight_);

	// ポストエフェクトはシーンをまたいで残るので、上書きした値を必ず戻す
	if (heatDistortion_) {
		heatDistortion_->GetEffectData().strength = 0.0f;
		heatDistortion_->SetActive(false);
	}
	if (colorGrading_) {
		colorGrading_->GetEffectData().strength = 0.0f;
		colorGrading_->SetActive(false);
	}
	if (bloom_ && bloomBoosted_) {
		bloom_->GetSettings().intensity = bloomBaseIntensity_;
	}
	if (flash_) {
		flash_->GetEffectData().flashColor.w = 0.0f;
		flash_->SetActive(false);
	}
	if (chroma_) {
		chroma_->GetEffectData().strength = 0.0f;
		chroma_->SetActive(false);
	}
}

void BossBreathEffect::Initialize(const std::string& ownerName) {
	ownerName_ = ownerName;
	rng_.seed(std::random_device{}());

	// 口元のライト。溜めで明るくなり、吐いている間は揺らぎ、余韻で消える
	auto light = std::make_unique<DynamicPointLight>(ownerName + "BreathLight");
	light->SetEnabled(false);
	light->SetDecay(kLightDecay);
	light_ = static_cast<DynamicPointLight*>(LightManager::GetInstance().AddLight(std::move(light)));

	// 炎の進行方向を照らすスポットライト。吐いている間だけ点く
	auto spotLight = std::make_unique<DynamicSpotLight>(ownerName + "BreathSpotLight");
	spotLight->SetEnabled(false);
	spotLight->SetColor(kFireColor);
	spotLight->SetDecay(kSpotDecay);
	spotLight->SetCosAngle(kSpotCosAngle);
	spotLight_ = static_cast<DynamicSpotLight*>(LightManager::GetInstance().AddLight(std::move(spotLight)));

	// 同名のパスが既にあれば AddEffect 側で弾かれる（シーンをまたいで生き続けるため）。
	// 登録した順にかかるので、絵を揺らす → 色を変える → 閃光 → 色収差 の順にする
	auto& offScreen = OffScreenManager::GetInstance();
	{
		auto heat = std::make_unique<HeatDistortionEffect>("BossBreathHeatDistortion");
		heat->SetActive(false);
		offScreen.AddEffect(std::move(heat));
	}
	{
		auto grading = std::make_unique<ColorGradingEffect>("BossBreathColorGrading");
		grading->SetActive(false);
		offScreen.AddEffect(std::move(grading));
	}
	{
		auto flash = std::make_unique<HitFlashEffect>("BossBreathFlash");
		flash->SetActive(false);
		offScreen.AddEffect(std::move(flash));
	}
	{
		auto chroma = std::make_unique<ChromaticAberrationEffect>("BossBreathChromaticAberration");
		chroma->SetActive(false);
		offScreen.AddEffect(std::move(chroma));
	}
	flash_ = dynamic_cast<HitFlashEffect*>(offScreen.FindEffect("BossBreathFlash"));
	chroma_ = dynamic_cast<ChromaticAberrationEffect*>(offScreen.FindEffect("BossBreathChromaticAberration"));
	bloom_ = dynamic_cast<BloomEffect*>(offScreen.FindEffect("Bloom"));
	heatDistortion_ = dynamic_cast<HeatDistortionEffect*>(offScreen.FindEffect("BossBreathHeatDistortion"));
	colorGrading_ = dynamic_cast<ColorGradingEffect*>(offScreen.FindEffect("BossBreathColorGrading"));

	// 前のボスが焼き付けたまま消えていても、白や色収差が残った状態で始まらないようにする
	if (heatDistortion_) {
		heatDistortion_->GetEffectData().strength = 0.0f;
		heatDistortion_->SetActive(false);
	}
	if (colorGrading_) {
		colorGrading_->GetEffectData().strength = 0.0f;
		colorGrading_->SetActive(false);
	}
	if (flash_) {
		flash_->GetEffectData().flashColor.w = 0.0f;
		flash_->SetActive(false);
	}
	if (chroma_) {
		chroma_->GetEffectData().strength = 0.0f;
		chroma_->SetActive(false);
	}
}

void BossBreathEffect::RequestCharge(float progress) {
	request_ = Request::Charge;
	chargeProgress_ = std::clamp(progress, 0.0f, 1.0f);
}

void BossBreathEffect::RequestFire(float fireProgress, float reach, float halfWidth) {
	request_ = Request::Fire;
	fireProgress_ = std::clamp(fireProgress, 0.0f, 1.0f);
	reach_ = reach;
	halfWidth_ = halfWidth;
}

void BossBreathEffect::RequestStop() {
	request_ = Request::Stop;
}

void BossBreathEffect::Update(Enemy& enemy, float deltaTime) {
	const float dt = std::clamp(deltaTime, 0.0f, kMaxStep);

	UpdateMouth(enemy);

	// ── 要求を段階へ反映 ──
	switch (request_) {
	case Request::Charge:
		requestLostTimer_ = 0.0f;
		if (phase_ != Phase::Charge) EnterCharge();
		break;
	case Request::Fire:
		requestLostTimer_ = 0.0f;
		if (phase_ != Phase::Fire) EnterFire(enemy);
		break;
	case Request::Stop:
		EnterTail();
		break;
	case Request::None:
		// 死亡演出などでステートの更新が止まった。出しっぱなしにせず余韻へ移す
		if (phase_ == Phase::Charge || phase_ == Phase::Fire) {
			requestLostTimer_ += dt;
			if (requestLostTimer_ >= kRequestLostTimeout) EnterTail();
		}
		break;
	}
	request_ = Request::None;

	phaseTimer_ += dt;
	switch (phase_) {
	case Phase::Charge: UpdateCharge(dt); break;
	case Phase::Fire:   UpdateFire(dt);   break;
	case Phase::Tail:   UpdateTail(dt);   break;
	case Phase::None:   break;
	}

	UpdateLight(dt);
	UpdatePostEffects(dt);
}

void BossBreathEffect::UpdateMouth(Enemy& enemy) {
	const Vector3 scale = enemy.GetWorldTransform()->GetWorldScale();
	ownerScale_ = scale.z;
	forward_ = enemy.GetForward();
	foot_ = enemy.GetFootPosition();

	// 口のジョイントの位置。ジョイントのスケルトン空間行列 × レンダラーのワールド行列（BoneAttachment と同じ式）
	if (BaseRenderer* renderer = enemy.GetRenderer(ownerName_)) {
		if (SkinnedInstance* instance = renderer->GetSkinnedInstance()) {
			if (const Joint* joint = instance->GetSkeleton()->FindJoint(kMouthJoint)) {
				const Matrix4x4 world = joint->skeletonSpaceMatrix * renderer->GetWorldTransform()->GetMatWorld();
				mouth_ = Vector3{ world.m[3][0], world.m[3][1], world.m[3][2] }
					+ forward_ * (kMouthForwardOffset * scale.z);
				return;
			}
		}
	}

	// ジョイントが無いモデル: 体の正面の口元あたり（以前の BossStateBreath の位置）
	mouth_ = enemy.GetWorldTransform()->GetWorldPos()
		+ Vector3{ 0.0f, kFallbackMouthHeight * scale.y, 0.0f }
		+ forward_ * (kFallbackMouthForward * scale.z);
}

void BossBreathEffect::UpdateAim() {
	if (reach_ <= 0.0f) {
		aimDirection_ = forward_;
		return;
	}
	// 帯の奥（kAimRatio）の地面を狙う。口の高さから少し下向きに吐くことになり、
	// 炎が地面を舐めて、その辺りに地面の炎と焦げ跡が出る
	const Vector3 target = foot_ + forward_ * (reach_ * kAimRatio);
	const Vector3 toTarget = target - mouth_;
	aimDirection_ = (Length(toTarget) > 0.001f) ? Normalize(toTarget) : forward_;
}

// ─────────────────────────────────────────────────────────────
// 段階の切り替え
// ─────────────────────────────────────────────────────────────

void BossBreathEffect::EnterCharge() {
	phase_ = Phase::Charge;
	phaseTimer_ = 0.0f;
	chargeEmitTimer_ = 0.0f;
}

void BossBreathEffect::EnterFire(Enemy& enemy) {
	phase_ = Phase::Fire;
	phaseTimer_ = 0.0f;
	// 最初のフレームから途切れずに出す
	coreEmitTimer_ = kCoreInterval;
	outerEmitTimer_ = kOuterInterval;
	groundEmitTimer_ = 0.0f;
	scorchEmitTimer_ = kScorchInterval;
	flickerPhase_ = 0.0f;

	UpdateAim();

	// ── 発射の瞬間: 閃光・炎の塊・衝撃波・火花・押し出される煙 ──
	ParticleManager::GetInstance().PlayVFX(kIgniteVfx, mouth_, aimDirection_);

	if (flash_) {
		flashTimer_ = kIgniteFlashDuration;
		flash_->GetEffectData().flashColor = { 1.0f, 0.85f, 0.6f, kIgniteFlashIntensity };
		flash_->SetActive(true);
	}
	AddShake(kIgniteShake);
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddFovPunch(kIgniteFovPunch);
	}
	igniteBoostTimer_ = kIgniteLightBoostDuration;
	enemy.FlashLight();
}

void BossBreathEffect::EnterTail() {
	if (phase_ != Phase::Charge && phase_ != Phase::Fire) return;

	tailFromFire_ = (phase_ == Phase::Fire);
	phase_ = Phase::Tail;
	phaseTimer_ = 0.0f;
	tailEmitTimer_ = 0.0f;
	tailStartIntensity_ = lightIntensity_;
}

// ─────────────────────────────────────────────────────────────
// 段階ごとの更新
// ─────────────────────────────────────────────────────────────

void BossBreathEffect::UpdateCharge(float deltaTime) {
	const float t = chargeProgress_;

	// 溜めが進むほど間隔を詰め、量を増やす（口へ吸い込まれる煙・漏れる火の粉・口の中の光）
	const float interval = kChargeIntervalStart + (kChargeIntervalEnd - kChargeIntervalStart) * t;
	chargeEmitTimer_ += deltaTime;
	if (chargeEmitTimer_ >= interval) {
		chargeEmitTimer_ = 0.0f;
		const Vector3 leakDirection = Normalize(forward_ + kUp * 0.3f);
		ParticleManager::GetInstance().PlayVFX(kChargeVfx, mouth_, leakDirection,
			1.0f + (kChargeCountScaleEnd - 1.0f) * t);
	}

	// 終盤は低く唸るように揺らす
	if (t > kChargeRumbleStart) {
		const float k = (t - kChargeRumbleStart) / (1.0f - kChargeRumbleStart);
		AddShake(kChargeRumble * k * deltaTime);
	}
}

void BossBreathEffect::UpdateFire(float deltaTime) {
	UpdateAim();

	// 終わり際: 外炎を先に止め、中心炎も減らす＝炎が細くなって途切れる
	const float thin = Smoothstep(kThinStart, 1.0f, fireProgress_);
	const float outerRate = 1.0f - Smoothstep(kThinStart, kOuterStop, fireProgress_);
	const float coreRate = 1.0f - kCoreThinRate * thin;

	auto& particles = ParticleManager::GetInstance();

	coreEmitTimer_ += deltaTime * coreRate;
	while (coreEmitTimer_ >= kCoreInterval) {
		coreEmitTimer_ -= kCoreInterval;
		particles.PlayVFX(kCoreVfx, mouth_, aimDirection_);
	}

	outerEmitTimer_ += deltaTime * outerRate;
	while (outerEmitTimer_ >= kOuterInterval) {
		outerEmitTimer_ -= kOuterInterval;
		particles.PlayVFX(kOuterVfx, mouth_, aimDirection_);
	}

	// 炎が地面に届いてから、地面を舐める炎と焦げ跡を出す
	if (phaseTimer_ >= kGroundDelay && reach_ > 0.0f) {
		groundEmitTimer_ += deltaTime * (0.4f + 0.6f * outerRate);
		while (groundEmitTimer_ >= kGroundInterval) {
			groundEmitTimer_ -= kGroundInterval;
			particles.PlayVFX(kGroundVfx, RandomBandPoint(0.55f, 1.0f, 0.6f, 0.05f), forward_);
		}

		scorchEmitTimer_ += deltaTime * outerRate;
		while (scorchEmitTimer_ >= kScorchInterval) {
			scorchEmitTimer_ -= kScorchInterval;
			// 上向きに寝かせて地面に貼る
			particles.PlayVFX(kScorchVfx, RandomBandPoint(0.4f, 1.0f, 0.5f, 0.0f), kUp);
		}
	}

	AddShake(kFireRumble * deltaTime * (1.0f - 0.6f * thin));
}

void BossBreathEffect::UpdateTail(float deltaTime) {
	const float duration = tailFromFire_ ? kTailDuration : kCancelFadeDuration;
	const float t = std::clamp(phaseTimer_ / duration, 0.0f, 1.0f);

	// 吐いた後だけ、火の粉と煙を少しずつ減らしながら残す
	if (tailFromFire_) {
		tailEmitTimer_ += deltaTime;
		if (tailEmitTimer_ >= kTailInterval) {
			tailEmitTimer_ = 0.0f;
			ParticleManager::GetInstance().PlayVFX(kTailVfx, mouth_, Normalize(forward_ + kUp * 0.4f),
				1.0f - 0.7f * t);
		}
	}

	if (t >= 1.0f) {
		phase_ = Phase::None;
		phaseTimer_ = 0.0f;
	}
}

void BossBreathEffect::UpdateLight(float deltaTime) {
	if (!light_) return;

	float intensity = 0.0f;
	float radius = kLightRadiusCharge;
	Vector4 color = kFireColor;
	Vector3 position = mouth_;

	switch (phase_) {
	case Phase::Charge: {
		// 赤 → 橙 → 白 と色が変わりながら、終盤で一気に明るくなる
		const float t = chargeProgress_;
		color = (t < 0.5f)
			? LerpColor(kChargeColorLow, kChargeColorMid, t / 0.5f)
			: LerpColor(kChargeColorMid, kChargeColorHigh, (t - 0.5f) / 0.5f);
		intensity = kChargeLightIntensity * t * t;
		radius = kLightRadiusCharge * (0.5f + 0.5f * t);
		break;
	}
	case Phase::Fire: {
		// 炎の揺らぎに合わせてちらつかせる。発射の瞬間はひときわ明るい
		flickerPhase_ += deltaTime;
		const float flicker = 1.0f
			+ 0.12f * std::sin(flickerPhase_ * 23.0f)
			+ 0.08f * std::sin(flickerPhase_ * 37.0f + 1.3f);
		const float thin = Smoothstep(kThinStart, 1.0f, fireProgress_);

		igniteBoostTimer_ = (std::max)(igniteBoostTimer_ - deltaTime, 0.0f);
		const float boost = igniteBoostTimer_ / kIgniteLightBoostDuration;

		intensity = kFireLightIntensity * flicker * (1.0f - 0.5f * thin) + kIgniteLightBoost * boost * boost;
		radius = kLightRadiusFire;
		position = mouth_ + aimDirection_ * (kFireLightForward * ownerScale_);
		break;
	}
	case Phase::Tail: {
		const float duration = tailFromFire_ ? kTailDuration : kCancelFadeDuration;
		const float k = 1.0f - std::clamp(phaseTimer_ / duration, 0.0f, 1.0f);
		intensity = tailStartIntensity_ * k * k;
		radius = tailFromFire_ ? kLightRadiusFire : kLightRadiusCharge;
		break;
	}
	case Phase::None:
		break;
	}

	lightIntensity_ = intensity;
	light_->SetEnabled(intensity > 0.001f);
	light_->SetColor(color);
	light_->SetIntensity(intensity);
	light_->SetRadius(radius * ownerScale_);
	light_->SetPosition(position);

	// ── 炎の進行方向を照らすスポットライト ──
	// 吐いている間と、その余韻だけ点ける（溜めの間は口元の光だけにして、吐いた瞬間の変化を大きくする）
	if (spotLight_) {
		const bool lit = (phase_ == Phase::Fire || (phase_ == Phase::Tail && tailFromFire_)) && reach_ > 0.0f;
		const float spotIntensity = lit ? intensity * kSpotIntensityRate : 0.0f;
		spotLight_->SetEnabled(spotIntensity > 0.001f);
		spotLight_->SetIntensity(spotIntensity);
		spotLight_->SetPosition(mouth_);
		spotLight_->SetDirection(aimDirection_);
		spotLight_->SetDistance(reach_ * kSpotDistanceRate);
	}
}

void BossBreathEffect::UpdatePostEffects(float deltaTime) {
	// ── 白い閃光（発射の瞬間だけ）──
	if (flash_ && flashTimer_ > 0.0f) {
		flashTimer_ -= deltaTime;
		const float k = std::clamp(flashTimer_ / kIgniteFlashDuration, 0.0f, 1.0f);
		flash_->GetEffectData().flashColor = { 1.0f, 0.85f, 0.6f, kIgniteFlashIntensity * k * k };
		if (flashTimer_ <= 0.0f) {
			flashTimer_ = 0.0f;
			flash_->GetEffectData().flashColor.w = 0.0f;
			flash_->SetActive(false);
		}
	}

	// ── ブレス中だけ効かせるもの（ブルーム増し・色収差）──
	float target = 0.0f;
	switch (phase_) {
	case Phase::Charge: target = kChargePostWeight * chargeProgress_; break;
	case Phase::Fire:   target = 1.0f; break;
	case Phase::Tail:
		target = tailFromFire_ ? 1.0f - std::clamp(phaseTimer_ / kTailDuration, 0.0f, 1.0f) : 0.0f;
		break;
	case Phase::None:   break;
	}
	postWeight_ += (target - postWeight_) * (std::min)(kPostFollowRate * deltaTime, 1.0f);
	if (target <= 0.0f && postWeight_ < 0.002f) {
		postWeight_ = 0.0f;
	}

	if (bloom_) {
		if (postWeight_ > 0.0f) {
			// 上書きを始める瞬間の値を覚えておき、終わったらそこへ戻す
			if (!bloomBoosted_) {
				bloomBaseIntensity_ = bloom_->GetSettings().intensity;
				bloomBoosted_ = true;
			}
			bloom_->GetSettings().intensity = bloomBaseIntensity_ + kBloomBoost * postWeight_;
		} else if (bloomBoosted_) {
			bloom_->GetSettings().intensity = bloomBaseIntensity_;
			bloomBoosted_ = false;
		}
	}

	if (chroma_) {
		// 色収差は炎を吐いている間だけ（溜めでかけると画面がずっと滲んで見える）
		const float fireWeight = (phase_ == Phase::Fire || (phase_ == Phase::Tail && tailFromFire_)) ? postWeight_ : 0.0f;
		const float strength = kChromaStrength * fireWeight;
		if (strength > 1e-5f) {
			Vector2 center{ 0.5f, 0.5f };
			ToScreenUV(mouth_, center);
			chroma_->GetEffectData().center = center;
			chroma_->GetEffectData().strength = strength;
			chroma_->SetActive(true);
		} else if (chroma_->IsActive()) {
			chroma_->GetEffectData().strength = 0.0f;
			chroma_->SetActive(false);
		}
	}

	// ── カラーグレーディング（ブレス中だけ暖色に寄せる）──
	if (colorGrading_) {
		const float strength = kGradingStrength * postWeight_;
		if (strength > 0.001f) {
			auto& data = colorGrading_->GetEffectData();
			data.strength = strength;
			data.gain = kGradingGain;
			data.lift = kGradingLift;
			data.saturation = kGradingSaturation;
			data.contrast = kGradingContrast;
			colorGrading_->SetActive(true);
		} else if (colorGrading_->IsActive()) {
			colorGrading_->GetEffectData().strength = 0.0f;
			colorGrading_->SetActive(false);
		}
	}

	// ── 熱の歪み ──
	// 溜めの間は口元だけ、吐いている間は口から炎の先までの帯を揺らす
	if (heatDistortion_) {
		float strength = 0.0f;
		Vector2 startUV{};
		Vector2 endUV{};
		float startRadius = 0.0f;
		float endRadius = 0.0f;

		if (phase_ == Phase::Charge) {
			if (ProjectToScreen(mouth_, kHeatChargeRadius * ownerScale_, startUV, startRadius)) {
				endUV = startUV;
				endRadius = startRadius;
				strength = kHeatChargeStrength * chargeProgress_;
			}
		} else if ((phase_ == Phase::Fire || (phase_ == Phase::Tail && tailFromFire_)) && reach_ > 0.0f) {
			const Vector3 bandEnd = foot_ + forward_ * (reach_ * kHeatEndRatio)
				+ kUp * (kHeatEndHeight * ownerScale_);
			if (ProjectToScreen(mouth_, kHeatMouthRadius * ownerScale_, startUV, startRadius)
				&& ProjectToScreen(bandEnd, halfWidth_ * kHeatEndWidthRate, endUV, endRadius)) {
				strength = kHeatFireStrength * postWeight_;
			}
		}

		if (strength > 1e-5f) {
			auto& data = heatDistortion_->GetEffectData();
			data.segmentStart = startUV;
			data.segmentEnd = endUV;
			data.radiusStart = startRadius;
			data.radiusEnd = endRadius;
			data.strength = strength;
			heatDistortion_->SetActive(true);
		} else if (heatDistortion_->IsActive()) {
			heatDistortion_->GetEffectData().strength = 0.0f;
			heatDistortion_->SetActive(false);
		}
	}
}

// ─────────────────────────────────────────────────────────────
// 補助
// ─────────────────────────────────────────────────────────────

Vector3 BossBreathEffect::RandomBandPoint(float nearRatio, float farRatio, float lateral, float height) {
	std::uniform_real_distribution<float> along(nearRatio, farRatio);
	std::uniform_real_distribution<float> side(-lateral, lateral);

	const Vector3 right{ forward_.z, 0.0f, -forward_.x };
	Vector3 point = foot_ + forward_ * (reach_ * along(rng_)) + right * (halfWidth_ * side(rng_));
	point.y = foot_.y + height;
	return point;
}

bool BossBreathEffect::ToScreenUV(const Vector3& worldPosition, Vector2& outUV) {
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera || !camera->IsInView(worldPosition)) return false;

	const Vector2 screen = camera->WorldToScreen(worldPosition,
		static_cast<int>(WindowManager::kGameWidth), static_cast<int>(WindowManager::kGameHeight));
	outUV = {
		screen.x / static_cast<float>(WindowManager::kGameWidth),
		screen.y / static_cast<float>(WindowManager::kGameHeight)
	};
	return true;
}

bool BossBreathEffect::ProjectToScreen(const Vector3& worldPosition, float worldRadius, Vector2& outUV, float& outRadius) {
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) return false;

	// 行ベクトル規約（v * VP）。画面の外でも、カメラの前にあれば帯の端として使えるので IsInView は見ない
	const Matrix4x4& vp = camera->GetViewProjectionMatrix();
	const Vector3& p = worldPosition;
	const float clipX = p.x * vp.m[0][0] + p.y * vp.m[1][0] + p.z * vp.m[2][0] + vp.m[3][0];
	const float clipY = p.x * vp.m[0][1] + p.y * vp.m[1][1] + p.z * vp.m[2][1] + vp.m[3][1];
	const float clipW = p.x * vp.m[0][3] + p.y * vp.m[1][3] + p.z * vp.m[2][3] + vp.m[3][3];
	if (clipW <= 0.05f) return false;

	outUV = { clipX / clipW * 0.5f + 0.5f, -clipY / clipW * 0.5f + 0.5f };
	// 射影行列の [1][1] は 1/tan(縦画角/2)。NDC の縦は2なので、UV（縦=1）では半分になる
	outRadius = worldRadius * camera->GetProjectionMatrix().m[1][1] * 0.5f / clipW;
	return true;
}

void BossBreathEffect::AddShake(float trauma) {
	if (trauma <= 0.0f) return;
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddShake(trauma);
	}
}
