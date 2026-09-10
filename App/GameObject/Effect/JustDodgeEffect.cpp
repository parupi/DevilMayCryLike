#include "JustDodgeEffect.h"
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/HitFlashEffect.h>
#include <Graphics/Rendering/Particle/ParticleManager.h>
#include <World3D/Camera/CameraManager.h>
#include <Audio/SoundManager.h>
#include "GameObject/Camera/GameCamera.h"
#include <algorithm>

void JustDodgeEffect::Initialize() {
	auto& offScreen = OffScreenManager::GetInstance();

	// 同名のパスが既にあれば AddEffect 側で弾かれる（シーンをまたいで生き続けるため）
	auto flash = std::make_unique<HitFlashEffect>("JustDodgeFlash");
	flash->SetActive(false);
	offScreen.AddEffect(std::move(flash));

	flash_ = static_cast<HitFlashEffect*>(offScreen.FindEffect("JustDodgeFlash"));

	// ポストエフェクトはシーンをまたぐので、白が焼き付いた状態で始まらないよう必ず切っておく
	Stop();
}

void JustDodgeEffect::Play(const Vector3& position, const Vector3& attackDirection, float shakeTrauma) {
	timer_ = 0.0f;
	isPlaying_ = true;

	// 白フラッシュ。立ち上がりを見せたいので最大値から始める
	if (flash_) {
		flash_->GetEffectData().flashColor = { 1.0f, 1.0f, 1.0f, kFlashIntensity };
		flash_->SetActive(true);
	}

	// 円形の衝撃波。攻撃してきた方向へ向けて立てる
	ParticleManager::GetInstance().PlayVFX(kVFXName, position, attackDirection);

	// 専用SE。ファイルが無ければ SoundManager 側で握りつぶされる
	SoundManager::GetInstance().PlaySE(kSEName, 0.9f);

	// カメラは小さく揺らす。回避は頻繁に使うので強くしすぎない（仕様書 §11）
	if (auto* camera = dynamic_cast<GameCamera*>(CameraManager::GetInstance().GetActiveCamera())) {
		camera->AddShake(shakeTrauma);
	}
}

void JustDodgeEffect::Update(float deltaTime) {
	if (!isPlaying_) return;

	timer_ += deltaTime;
	if (timer_ >= kFlashDuration) {
		Stop();
		return;
	}

	// 発生直後が最大で、そこからイーズアウトで抜ける
	const float t = std::clamp(timer_ / kFlashDuration, 0.0f, 1.0f);
	const float eased = (1.0f - t) * (1.0f - t);

	if (flash_) {
		flash_->GetEffectData().flashColor = { 1.0f, 1.0f, 1.0f, kFlashIntensity * eased };
	}
}

void JustDodgeEffect::Stop() {
	isPlaying_ = false;
	timer_ = 0.0f;

	// 使わない間はパスごと無効にして描画コストを0にする
	if (flash_) {
		flash_->GetEffectData().flashColor.w = 0.0f;
		flash_->SetActive(false);
	}
}
