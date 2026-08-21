#include "DeathScreenEffect.h"

#include <Graphics/Rendering/PostEffect/GrayEffect.h>
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
#include <Math/MathUtils.h>

#include <algorithm>
#include <memory>

void DeathScreenEffect::Initialize() {
	OffScreenManager& offScreen = OffScreenManager::GetInstance();

	// AddEffect は同じ名前を弾くので、2回目以降のシーンでは既存のものが見つかる
	if (!offScreen.FindEffect(kGrayName)) {
		auto gray = std::make_unique<GrayEffect>(kGrayName);
		gray->SetActive(false);
		offScreen.AddEffect(std::move(gray));
	}
	gray_ = static_cast<GrayEffect*>(offScreen.FindEffect(kGrayName));

	if (!offScreen.FindEffect(kVignetteName)) {
		auto vignette = std::make_unique<VignetteEffect>(kVignetteName);
		vignette->SetActive(false);
		vignette->SetColor(0.0f, 0.0f, 0.0f); // 死亡の暗転は黒
		offScreen.AddEffect(std::move(vignette));
	}
	vignette_ = static_cast<VignetteEffect*>(offScreen.FindEffect(kVignetteName));

	// 前のプレイで閉じたままになっている可能性があるので、必ず開けておく
	Reset();
}

void DeathScreenEffect::SetProgress(float progress) {
	const float t = std::clamp(progress, 0.0f, 1.0f);
	// 効いていない間はパスごと止める（全画面パスを2つ余計に走らせないため）
	const bool active = t > 0.001f;

	if (gray_) {
		gray_->SetActive(active);
		if (auto* data = gray_->GetEffectData()) {
			data->intensity = kMaxGray * t;
		}
	}

	if (vignette_) {
		vignette_->SetActive(active);
		VignetteEffect::VignetteEffectData& data = vignette_->GetEffectData();
		data.intensity = kMaxIntensity * t;
		data.radius = Lerp(kStartRadius, kEndRadius, t);
		data.softness = kSoftness;
	}
}
