#include "HitPostEffect.h"
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/RadialBlurEffect.h>
#include <Graphics/Rendering/PostEffect/ChromaticAberrationEffect.h>
#include <Graphics/Rendering/PostEffect/HitFlashEffect.h>
#include <algorithm>
#ifdef _DEBUG
#endif

// 弱いほど短く浅く、強いほど長く深くかける
const HitPostEffect::Preset HitPostEffect::kPresets[static_cast<size_t>(HitStopStrength::Count)] = {
	// duration, blur,   chroma,  flash
	{  0.06f,    0.010f, 0.0015f, 0.05f }, // Light
	{  0.09f,    0.022f, 0.0035f, 0.12f }, // Medium
	{  0.13f,    0.038f, 0.0060f, 0.25f }, // Heavy
};

void HitPostEffect::Initialize() {
	auto& offScreen = OffScreenManager::GetInstance();

	// 登録した順にチェーンされる。ブラー → 色収差 → フラッシュ の順にかける
	auto radialBlur = std::make_unique<RadialBlurEffect>("HitRadialBlur");
	radialBlur->SetActive(false);
	offScreen.AddEffect(std::move(radialBlur));

	auto chroma = std::make_unique<ChromaticAberrationEffect>("HitChromaticAberration");
	chroma->SetActive(false);
	offScreen.AddEffect(std::move(chroma));

	auto flash = std::make_unique<HitFlashEffect>("HitFlash");
	flash->SetActive(false);
	offScreen.AddEffect(std::move(flash));

	radialBlur_ = static_cast<RadialBlurEffect*>(offScreen.FindEffect("HitRadialBlur"));
	chroma_ = static_cast<ChromaticAberrationEffect*>(offScreen.FindEffect("HitChromaticAberration"));
	flash_ = static_cast<HitFlashEffect*>(offScreen.FindEffect("HitFlash"));
}

void HitPostEffect::Play(HitStopStrength strength, const Vector2& screenUV) {
	const Preset& preset = kPresets[static_cast<size_t>(HitStop::ToStrength(static_cast<int32_t>(strength)))];

	// 連続ヒット中に弱い攻撃が入っても、再生中の強い演出は潰さない
	if (isPlaying_ && preset.blurStrength < current_.blurStrength) {
		center_ = screenUV;
		return;
	}

	current_ = preset;
	center_ = screenUV;
	timer_ = 0.0f;
	isPlaying_ = true;

	Apply(1.0f);
}

void HitPostEffect::Stop() {
	isPlaying_ = false;
	timer_ = 0.0f;

	// 使わない間はパスごと無効にして描画コストを0にする
	if (radialBlur_) {
		radialBlur_->GetEffectData().strength = 0.0f;
		radialBlur_->SetActive(false);
	}
	if (chroma_) {
		chroma_->GetEffectData().strength = 0.0f;
		chroma_->SetActive(false);
	}
	if (flash_) {
		flash_->GetEffectData().flashColor.w = 0.0f;
		flash_->SetActive(false);
	}
}

void HitPostEffect::Update(float deltaTime) {
	if (!isPlaying_) return;

	timer_ += deltaTime;

	if (timer_ >= current_.duration || current_.duration <= 0.0f) {
		Stop();
		return;
	}

	// ヒット直後が最大で、そこからイーズアウトで消える
	const float t = std::clamp(timer_ / current_.duration, 0.0f, 1.0f);
	const float eased = (1.0f - t) * (1.0f - t);

	Apply(eased);
}

void HitPostEffect::Apply(float eased) {
	if (radialBlur_) {
		radialBlur_->GetEffectData().center = center_;
		radialBlur_->GetEffectData().strength = current_.blurStrength * eased;
		radialBlur_->SetActive(true);
	}
	if (chroma_) {
		chroma_->GetEffectData().center = center_;
		chroma_->GetEffectData().strength = current_.chromaStrength * eased;
		chroma_->SetActive(true);
	}
	if (flash_) {
		flash_->GetEffectData().flashColor = { kFlashColorR, kFlashColorG, kFlashColorB, current_.flashIntensity * eased };
		flash_->SetActive(true);
	}
}
