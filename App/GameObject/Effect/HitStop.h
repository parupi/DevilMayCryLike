#pragma once
#include "math/Vector3.h"
#include <random>
class HitStop
{
private:
	struct HitStopData {
		bool isActive = false;
		Vector3 translate{};
	}hitStopData_;
public:
	HitStop() = default;
	~HitStop() = default;

	void Update();

	void Start(float time, float intensity);

	HitStopData GetHitStopData() const { return hitStopData_; }
private:
	float intensity_;
	float maxTime_;
	float currentTimer_;
};

