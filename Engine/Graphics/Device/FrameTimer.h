#pragma once
#include <chrono>

/// <summary>
/// フレームレートの上限を掛けるタイマー。
///
/// 既定では上限なし＝ vsync（SwapChain の Present）に任せる。
/// ソフト側で60FPSに待たせると、リフレッシュレートが60の倍数でないディスプレイ
/// （例: 144Hz なら vsync は 6.94ms 刻み）で待ち時間が噛み合わず、
/// フレーム時間が2枠と3枠を行き来して 55〜57FPS のように揺れてしまう。
/// 上限が要る場合だけ SetTargetFps() で入れること。
/// </summary>
class FrameTimer {
public:
	void Initialize();
	/// <summary>上限FPSに達するまで待つ。上限なし（0以下）のときは何もしない</summary>
	void Update();

	/// <summary>上限FPSを設定する。0以下で上限なし（vsync任せ）</summary>
	void SetTargetFps(float fps) { targetFps_ = fps; }
	float GetTargetFps() const { return targetFps_; }

private:
	std::chrono::steady_clock::time_point reference_;
	// 上限FPS。0以下で無制限
	float targetFps_ = 0.0f;
};
