#include "HitStop.h"
#include <algorithm>

float HitStop::ToTimeScale(HitStopStrength strength)
{
	return kTimeScaleTable[static_cast<size_t>(ToStrength(static_cast<int32_t>(strength)))];
}

HitStopStrength HitStop::ToStrength(int32_t value)
{
	constexpr int32_t kMax = static_cast<int32_t>(HitStopStrength::Count) - 1;
	return static_cast<HitStopStrength>(std::clamp(value, 0, kMax));
}

void HitStop::Update(float deltaTime)
{
	if (!hitStopData_.isActive) {
		hitStopData_.timeScale = 1.0f;
		return;
	}

	// timerはscene時間で進める
	timer_ += deltaTime;
	hitStopData_.progress = timer_ / maxTime_;

	if (timer_ < maxTime_) {
		// --- TimeScale ---
		hitStopData_.timeScale = stopScale_;

		// --- Shake ---
		float t = 1.0f - hitStopData_.progress;
		hitStopData_.translate = { dist(mt) * intensity_ * t, dist(mt) * intensity_ * t, 0.0f };
	} else {
		hitStopData_.isActive = false;
		hitStopData_.translate = {};
		hitStopData_.progress = 1.0f;
		hitStopData_.timeScale = 1.0f;
	}
}

void HitStop::Start(float time, float intensity, HitStopStrength strength)
{
	// ヒットストップ無しの攻撃は何もしない（発生中なら今のストップをそのまま継続させる）
	if (time <= 0.0f) return;

	// 攻撃の強さからヒットストップ中のタイムスケールを決める
	const float timeScale = ToTimeScale(strength);

	if (hitStopData_.isActive) {
		// 発生中に重ねてヒットした場合は「長く・強く(より止まる方)」を優先する
		maxTime_ = std::max(maxTime_, time);
		intensity_ = std::max(intensity_, intensity);
		stopScale_ = std::min(stopScale_, timeScale);
	} else {
		maxTime_ = time;
		intensity_ = intensity;
		stopScale_ = timeScale;
	}

	timer_ = 0.0f;

	hitStopData_.isActive = true;
	hitStopData_.translate = {};
	hitStopData_.progress = 0.0f;
	hitStopData_.timeScale = stopScale_;
}
