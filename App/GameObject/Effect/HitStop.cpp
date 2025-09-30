#include "HitStop.h"
#include <base/utility/DeltaTime.h>
#include <cstdlib> 

void HitStop::Update()
{
	if (!hitStopData_.isActive) return;

	currentTimer_ += 1.0f / maxTime_ * DeltaTime::GetDeltaTime();

	// まだHitStop時間中なら揺れを生成
	if (currentTimer_ < maxTime_)
	{
		// ランダムに -1.0f～1.0f の範囲を作る
		float rx = (float(rand()) / RAND_MAX) * 2.0f - 1.0f;
		float ry = (float(rand()) / RAND_MAX) * 2.0f - 1.0f;

		// 残り時間が少なくなるにつれて揺れを弱める
		float t = 1.0f - (currentTimer_ / maxTime_);
		hitStopData_.translate = Vector3{ rx * intensity_ * t, ry * intensity_ * t, 0.0f };
	} else
	{
		// HitStop終了
		hitStopData_.isActive = false;
		hitStopData_.translate = {};
	}
}

void HitStop::Start(float time, float intensity)
{
	maxTime_ = time;
	intensity_ = intensity;
	currentTimer_ = 0.0f;

	hitStopData_.isActive = true;
	hitStopData_.translate = {};
}
