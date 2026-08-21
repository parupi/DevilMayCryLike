#include "FrameTimer.h"
#include <thread>

void FrameTimer::Initialize()
{
	reference_ = std::chrono::steady_clock::now();
}

void FrameTimer::Update()
{
	const auto now = std::chrono::steady_clock::now();

	// 上限なし。待たずに基準だけ進めて vsync に任せる
	if (targetFps_ <= 0.0f) {
		reference_ = now;
		return;
	}

	const std::chrono::microseconds frameTime(uint64_t(1000000.0f / targetFps_));
	const auto deadline = reference_ + frameTime;

	// 大きく遅れたフレームの後は取り返そうとせず、そこを新しい基準にする
	if (now > deadline + frameTime) {
		reference_ = now;
		return;
	}

	// sleep は 1ms 前後ずれるので、締切の手前までは寝て、残りは譲りながら回して待つ。
	// 全部 sleep で待つと毎フレーム 1ms 弱オーバーして、狙ったFPSより少し下で落ち着いてしまう
	constexpr std::chrono::microseconds kSpinMargin(1500);
	while (true) {
		const auto current = std::chrono::steady_clock::now();
		if (current >= deadline) break;
		if (deadline - current > kSpinMargin) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		} else {
			std::this_thread::yield();
		}
	}

	// 実際の起床時刻ではなく締切を次の基準にして、待ちの誤差が積み上がらないようにする
	reference_ = deadline;
}
