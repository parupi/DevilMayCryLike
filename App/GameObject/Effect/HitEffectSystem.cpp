#include "HitEffectSystem.h"
#include "GameObject/Camera/GameCamera.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Effect/CharacterLight.h"
#include "GameObject/Effect/HitPostEffect.h"
#include "Debugger/GlobalVariables.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Platform/WindowManager.h"
#include "World3D/Camera/CameraManager.h"
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
			request.vfxName, request.position, request.direction, preset.particleScale);
	}

	// ② カメラシェイク
	if (camera_) {
		camera_->AddShake(preset.shakeTrauma);
	}

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

