#pragma once
#include <chrono>
class DeltaTime
{
public:
	// 初期化
	static void Initialize();
	// 更新
	static void Update();

	// 取得
	static float GetDeltaTime() {return deltaTime_;}

private:
	static std::chrono::high_resolution_clock::time_point preTime_;
	static float deltaTime_;
};

