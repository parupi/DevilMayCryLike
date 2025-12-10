#include "HitStop.h"
#include <base/utility/DeltaTime.h>

void HitStop::Update()
{
	if (!hitStopData_.isActive) return;

	timer_ += DeltaTime::GetDeltaTime();
	hitStopData_.progress = timer_ / maxTime_;

	if (timer_ < maxTime_)
	{
		float t = 1.0f - hitStopData_.progress;
		hitStopData_.translate = Vector3{ dist(mt) * intensity_ * t, dist(mt) * intensity_ * t, 0.0f };
	} else
	{
		hitStopData_.isActive = false;
		hitStopData_.translate = {};
		hitStopData_.progress = 1.0f;
	}
}

void HitStop::Start(float time, float intensity)
{
	maxTime_ = time;
	intensity_ = intensity;
	timer_ = 0.0f;

	hitStopData_.isActive = true;
	hitStopData_.translate = {};
	hitStopData_.progress = 0.0f;
}
