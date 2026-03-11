#include "HitStop.h"

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

void HitStop::Start(float time, float intensity, float stopScale)
{
	// 新しいヒットストップと現在のを比べて長いほうを設定する
	maxTime_ = std::max(maxTime_, time);
	intensity_ = intensity;
	stopScale_ = stopScale;
	timer_ = 0.0f;

	hitStopData_.isActive = true;
	hitStopData_.translate = {};
	hitStopData_.progress = 0.0f;
	hitStopData_.timeScale = stopScale_;
}