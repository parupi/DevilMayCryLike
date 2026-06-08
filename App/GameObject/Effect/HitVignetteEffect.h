#pragma once

/// <summary>
/// プレイヤーが被弾したときに赤いビネットを一瞬フラッシュさせるエフェクト。
/// OffScreenManager に VignetteEffect を登録し、intensity を時間経過で減衰させる。
/// </summary>
class HitVignetteEffect {
public:
	// 初期化（OffScreenManager に VignetteEffect を追加する）
	void Initialize();
	// 毎フレーム呼ぶ
	void Update(float deltaTime);
	// 被弾時に呼ぶ
	void Play();
	// 死亡時にとめる
	void Stop();

private:
	class VignetteEffect* vignette_ = nullptr;

	bool  isPlaying_ = false;
	float timer_ = 0.0f;

	// ----- チューニングパラメータ -----
	static constexpr float kFlashIntensity = 0.85f; // 被弾直後の強度
	static constexpr float kRadius = 0.45f; // ビネットの開始距離
	static constexpr float kSoftness = -0.25f; // フェードの滑らかさ
	static constexpr float kDuration = 1.2f;  // 完全に消えるまでの秒数
	static constexpr float kColorR = 0.75f; // エッジ色 R
	static constexpr float kColorG = 0.0f;
	static constexpr float kColorB = 0.0f;
};
