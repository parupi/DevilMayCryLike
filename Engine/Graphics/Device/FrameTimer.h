#pragma once
#include <chrono>

// フレームレートを60FPSに固定するタイマー
class FrameTimer {
public:
	void Initialize();
	void Update();

private:
	std::chrono::steady_clock::time_point reference_;
};
