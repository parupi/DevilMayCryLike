#include "HitVignetteEffect.h"
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <algorithm>

void HitVignetteEffect::Initialize() {
	auto vignette = std::make_unique<VignetteEffect>("HitVignette");
	vignette->SetActive(true);
	vignette->GetEffectData().radius = kRadius;
	vignette->GetEffectData().intensity = 0.0f; // 初期は無効
	vignette->GetEffectData().softness = kSoftness;
	vignette->SetColor(kColorR, kColorG, kColorB);

	OffScreenManager::GetInstance()->AddEffect(std::move(vignette));

	vignette_ = static_cast<VignetteEffect*>(
		OffScreenManager::GetInstance()->FindEffect("HitVignette"));
}

void HitVignetteEffect::Play() {
	if (!vignette_) return;

	isPlaying_ = true;
	timer_ = 0.0f;
	vignette_->GetEffectData().intensity = kFlashIntensity;
}

void HitVignetteEffect::Stop() {
	if (!vignette_) return;

	isPlaying_ = false;
	timer_ = 0.0f;
	vignette_->GetEffectData().intensity = 0.0f;
}

void HitVignetteEffect::Update(float deltaTime) {
	if (!isPlaying_ || !vignette_) return;

	timer_ += deltaTime;

	// intensity を kFlashIntensity → 0 へイーズアウト
	float t = (std::min)(timer_ / kDuration, 1.0f);
	float eased = 1.0f - t * t; // ease-out quad
	vignette_->GetEffectData().intensity = kFlashIntensity * eased;

	if (timer_ >= kDuration) {
		vignette_->GetEffectData().intensity = 0.0f;
		isPlaying_ = false;
	}
}
