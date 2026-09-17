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

	// 実測の経過秒。エディタの一時停止や速度倍率の影響を受けない。
	// 止まっていても動かしたいもの（エディタ自身の計測、メニューの演出、音のフェード）はこちらを使う。
	// Release にはデバッグ用のスケールが無いので GetDeltaTime と同じ値になる
#ifdef _DEBUG
	static float GetUnscaledDeltaTime() { return unscaledDeltaTime_; }
#else
	static float GetUnscaledDeltaTime() { return deltaTime_; }
#endif // _DEBUG

#ifdef _DEBUG
	// --- エディタの再生コントロール ---
	// ここで時間を止めたり伸ばしたりすると、TimeManager を含む下流すべてに効く。
	// ゲーム側のコードは一切変更しなくてよい。

	// 一時停止する／解除する
	static void SetPaused(bool paused) { paused_ = paused; }
	static bool IsPaused() { return paused_; }
	// 一時停止中でも指定フレーム数だけ進める（コマ送り）
	static void RequestStep(int frames = 1) { stepFrames_ += frames; }
	// 通常再生時の速度倍率
	static void SetDebugTimeScale(float scale) { debugTimeScale_ = scale; }
	static float GetDebugTimeScale() { return debugTimeScale_; }
#endif // _DEBUG

private:
	// 1フレームで進める時間の上限（秒）。
	// これより長く止まっていた場合でも、ゲーム側にはここまでしか渡さない
	static constexpr float kMaxDeltaTime = 0.1f;

	static std::chrono::high_resolution_clock::time_point preTime_;
	static float deltaTime_;

#ifdef _DEBUG
	// デバッグ用スケールを掛ける前の実測値
	static float unscaledDeltaTime_;
	static bool paused_;
	static int stepFrames_;
	static float debugTimeScale_;
	// コマ送り1回あたりに進める時間。止めていた実時間をそのまま入れると
	// 1コマで何秒も飛んでしまうので、固定値にしている
	static constexpr float kStepDeltaTime = 1.0f / 60.0f;
#endif // _DEBUG
};

