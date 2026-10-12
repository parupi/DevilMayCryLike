#include "HitEffectSystem.h"
#include "GameObject/Camera/GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Effect/CharacterLight.h"
#include "GameObject/Effect/HitPostEffect.h"
#include "Debugger/GlobalVariables.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Platform/WindowManager.h"
#include "World3D/Camera/CameraManager.h"
#include "Input/GamepadRumble.h"
#ifdef _DEBUG
#include <imgui/imgui.h>
#endif // _DEBUG

namespace {
	// 強さごとの既定値。ゲーム内で調整したものは GlobalVariables 側に保存される
	struct DefaultPreset {
		float particleScale;
		float shakeTrauma;
	};

	// Light / Medium / Heavy
	constexpr DefaultPreset kDefaults[static_cast<size_t>(HitStopStrength::Count)] = {
		{ 0.6f, 0.10f }, // Light  : 軽く小突いた程度
		{ 1.0f, 0.22f }, // Medium : 通常攻撃
		{ 1.8f, 0.40f }, // Heavy  : フィニッシュ・大技
	};

	// 弾かれたヒット。攻撃が通っていないので演出は控えめにする
	constexpr DefaultPreset kArmorDefault = { 0.5f, 0.08f };

	const char* StrengthName(HitStopStrength strength) {
		switch (HitStop::ToStrength(static_cast<int32_t>(strength))) {
		case HitStopStrength::Light:  return "Light";
		case HitStopStrength::Medium: return "Medium";
		case HitStopStrength::Heavy:  return "Heavy";
		default:                      return "Medium";
		}
	}
}

HitEffectSystem& HitEffectSystem::GetInstance()
{
	static HitEffectSystem instance;
	return instance;
}

std::string HitEffectSystem::MakeKey(HitStopStrength strength, bool isArmorHit, const char* suffix)
{
	return std::string(isArmorHit ? "Armor" : StrengthName(strength)) + suffix;
}

void HitEffectSystem::Initialize(GameCamera* camera, Player* player)
{
	camera_ = camera;
	player_ = player;

	global_ = &GlobalVariables::GetInstance();
	// 保存済みの調整値を先に読む（ディレクトリが無ければ何もしない）
	global_->LoadFiles("Effect");

	// 強さごとの演出量を登録する。
	// AddItem は既存キーを上書きしないので、調整済みの値があればそれが残る
	for (int32_t i = 0; i < static_cast<int32_t>(HitStopStrength::Count); ++i) {
		const auto strength = static_cast<HitStopStrength>(i);
		const DefaultPreset& def = kDefaults[i];
		global_->AddItem(kGroupName, MakeKey(strength, false, "ParticleScale"), def.particleScale);
		global_->AddItem(kGroupName, MakeKey(strength, false, "ShakeTrauma"), def.shakeTrauma);
	}
	global_->AddItem(kGroupName, "ArmorParticleScale", kArmorDefault.particleScale);
	global_->AddItem(kGroupName, "ArmorShakeTrauma", kArmorDefault.shakeTrauma);

	// 当たった瞬間のカメラの寄り（負の値で画角が狭まる）。強攻撃だけ一瞬寄る
	constexpr float kFovPunchDefaults[static_cast<size_t>(HitStopStrength::Count)] = { 0.0f, 0.0f, -0.035f };
	for (int32_t i = 0; i < static_cast<int32_t>(HitStopStrength::Count); ++i) {
		global_->AddItem(kGroupName, MakeKey(static_cast<HitStopStrength>(i), false, "FovPunch"), kFovPunchDefaults[i]);
	}
	global_->AddItem(kGroupName, "ArmorFovPunch", 0.0f);

	// コントローラーの振動。攻撃ごとの強さ・長さは攻撃データ（攻撃エディタの Rumble）にあり、
	// ここにあるのは全攻撃に掛ける倍率・終わりぎわの弱め方・弾かれたときの振動だけ
	global_->AddItem(kGroupName, "RumbleGain", 1.0f);
	global_->AddItem(kGroupName, "RumbleFadeRatio", 0.4f);
	// 弾かれたとき：重さの無い、短く鋭い揺れ（攻撃が通っていない手応え）
	global_->AddItem(kGroupName, "ArmorRumbleLow", 0.0f);
	global_->AddItem(kGroupName, "ArmorRumbleHigh", 0.45f);
	global_->AddItem(kGroupName, "ArmorRumbleDuration", 0.07f);
}

void HitEffectSystem::Finalize()
{
	// カメラとプレイヤーはシーンと一緒に破棄されるので参照を切っておく
	camera_ = nullptr;
	player_ = nullptr;
}

HitEffectSystem::Preset HitEffectSystem::LoadPreset(HitStopStrength strength, bool isArmorHit) const
{
	Preset preset{};
	if (!global_) {
		const DefaultPreset& def = isArmorHit
			? kArmorDefault
			: kDefaults[static_cast<size_t>(HitStop::ToStrength(static_cast<int32_t>(strength)))];
		preset.particleScale = def.particleScale;
		preset.shakeTrauma = def.shakeTrauma;
		return preset;
	}

	preset.particleScale = global_->GetValueRef<float>(kGroupName, MakeKey(strength, isArmorHit, "ParticleScale"));
	preset.shakeTrauma = global_->GetValueRef<float>(kGroupName, MakeKey(strength, isArmorHit, "ShakeTrauma"));
	return preset;
}

void HitEffectSystem::PlayRumble(const HitEffectRequest& request)
{
	RumbleRequest rumble;
	if (request.isArmorHit) {
		rumble.low = global_ ? global_->GetValueRef<float>(kGroupName, "ArmorRumbleLow") : 0.0f;
		rumble.high = global_ ? global_->GetValueRef<float>(kGroupName, "ArmorRumbleHigh") : 0.45f;
		rumble.duration = global_ ? global_->GetValueRef<float>(kGroupName, "ArmorRumbleDuration") : 0.07f;
	} else {
		rumble.low = request.rumbleLow;
		rumble.high = request.rumbleHigh;
		rumble.duration = request.rumbleDuration;
	}

	const float gain = global_ ? global_->GetValueRef<float>(kGroupName, "RumbleGain") : 1.0f;
	rumble.low *= gain;
	rumble.high *= gain;
	rumble.fadeRatio = global_ ? global_->GetValueRef<float>(kGroupName, "RumbleFadeRatio") : 0.4f;

	GamepadRumble::GetInstance().Play(rumble);
}

Vector2 HitEffectSystem::CalcScreenUV(const Vector3& worldPosition)
{
	const Vector2 kScreenCenter = { 0.5f, 0.5f };

	BaseCamera* camera = CameraManager::GetInstance().GetCurrentCamera();
	if (!camera) return kScreenCenter;
	if (!camera->IsInView(worldPosition)) return kScreenCenter;

	const Vector2 screenPos = camera->WorldToScreen(
		worldPosition,
		static_cast<int>(WindowManager::kGameWidth),
		static_cast<int>(WindowManager::kGameHeight));

	return {
		screenPos.x / static_cast<float>(WindowManager::kGameWidth),
		screenPos.y / static_cast<float>(WindowManager::kGameHeight)
	};
}

void HitEffectSystem::Play(const HitEffectRequest& request)
{
	const Preset preset = LoadPreset(request.strength, request.isArmorHit);

	// ① VFX（火花・リング・煙をまとめたエミッターをワンショット再生）
	//    未登録の名前なら PlayVFX が false を返して何も起きない
	if (!request.vfxName.empty()) {
		ParticleManager::GetInstance().PlayVFX(
			request.vfxName, request.position, request.direction, preset.particleScale, request.sizeScale);
	}
	// 当たった相手の材質ごとの破片（骨の欠片・硬い鱗の火花など）
	if (!request.materialVfxName.empty()) {
		ParticleManager::GetInstance().PlayVFX(
			request.materialVfxName, request.position, request.direction, preset.particleScale, request.sizeScale);
	}

	// ② カメラシェイクと、強攻撃の一瞬の寄り
	if (camera_) {
		camera_->AddShake(preset.shakeTrauma);
		const float fovPunch = global_
			? global_->GetValueRef<float>(kGroupName, MakeKey(request.strength, request.isArmorHit, "FovPunch"))
			: 0.0f;
		if (fovPunch != 0.0f) {
			camera_->AddFovPunch(fovPunch);
		}
	}

	// ⑥ コントローラーの振動。弾かれたときは攻撃の値ではなく共通の「弾かれた」振動にする
	PlayRumble(request);

	if (!player_) return;

	// ③ ヒット位置を中心にしたポストエフェクト（放射ブラー・色収差・フラッシュ）
	if (auto* postEffect = player_->GetHitPostEffect()) {
		postEffect->Play(request.strength, CalcScreenUV(request.position));
	}

	// ④ プレイヤー側のヒットストップ（長さ・揺れは攻撃データ由来）
	if (auto* hitStop = player_->GetHitStop()) {
		hitStop->Start(request.hitStopTime, request.hitStopIntensity, request.strength);
	}

	// ⑤ プレイヤーに追従するライトを強く光らせる
	if (auto* light = player_->GetCharacterLight()) {
		light->Flash();
	}
}

