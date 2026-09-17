#include "DeltaTime.h"
#include <thread>
// 表示は Engine/Editor/Windows/TimeWindow.cpp が担当する

std::chrono::high_resolution_clock::time_point DeltaTime::preTime_;
float DeltaTime::deltaTime_ = 0.0f;

#ifdef _DEBUG
float DeltaTime::unscaledDeltaTime_ = 0.0f;
bool DeltaTime::paused_ = false;
int DeltaTime::stepFrames_ = 0;
float DeltaTime::debugTimeScale_ = 1.0f;
#endif // _DEBUG

void DeltaTime::Initialize()
{
    preTime_ = std::chrono::high_resolution_clock::now();
	deltaTime_ = 0.0f;
}

void DeltaTime::Update()
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = currentTime - preTime_;
	const float rawMeasured = static_cast<float>(elapsed.count()); // 秒で保持
	preTime_ = currentTime;

	// シーンのロードやブレークポイントで数秒止まると、その分が丸ごと次の1フレームに乗る。
	// そのまま流すと演出が一瞬で終わったり、当たり判定がすり抜けたりするので上限を設ける。
	// 通常のフレーム(16ms程度)には影響しない
	const float measured = (rawMeasured > kMaxDeltaTime) ? kMaxDeltaTime : rawMeasured;

#ifdef _DEBUG
	// 実測値はエディタの計測用なので、こちらは丸めない
	unscaledDeltaTime_ = rawMeasured;

	if (stepFrames_ > 0) {
		// コマ送り。止まっていた実時間ではなく固定量だけ進める
		deltaTime_ = kStepDeltaTime;
		--stepFrames_;
	} else if (paused_) {
		deltaTime_ = 0.0f;
	} else {
		deltaTime_ = measured * debugTimeScale_;
	}
#else
	deltaTime_ = measured;
#endif // _DEBUG
}
