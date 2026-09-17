#pragma once
#include <string>

class GrayEffect;
class VignetteEffect;

/// <summary>
/// プレイヤーの死亡演出で画面から色を抜き、視界を閉じていくエフェクト。
/// グレースケールと黒いビネットの2つを、進行度（0〜1）ひとつでまとめて動かす。
///
/// ポストエフェクトは OffScreenManager がシーンをまたいで持ち続けるので、
/// シーンを作り直すたびに Initialize() を呼んで必ず切った状態へ戻すこと
/// （戻さないとリトライした画面が灰色のまま始まる）。
/// </summary>
class DeathScreenEffect {
public:
	/// <summary>OffScreenManager にエフェクトを登録し、効果を切った状態にする</summary>
	void Initialize();

	/// <summary>0 = 通常の画面 / 1 = 色が抜けて視界が閉じきった状態</summary>
	void SetProgress(float progress);

	/// <summary>効果を完全に切って通常の画面へ戻す</summary>
	void Reset() { SetProgress(0.0f); }

private:
	// OffScreenManager 上での名前。被弾時の赤いビネット（HitVignette）とは別物
	static constexpr const char* kGrayName = "DeathGray";
	static constexpr const char* kVignetteName = "DeathVignette";

	// ── 効き具合の調整つまみ（進行度1のときの値）──
	static constexpr float kMaxGray = 0.9f;        // ほぼモノクロまで色を抜く
	static constexpr float kMaxIntensity = 0.85f;  // ビネットの濃さ
	static constexpr float kStartRadius = 0.9f;    // 進行度0の視界の広さ（実質かかっていない）
	static constexpr float kEndRadius = 0.3f;      // 閉じきったときの視界の広さ
	static constexpr float kSoftness = -0.3f;      // 縁のぼかし幅（負の値で外へ向かって暗くなる）

	GrayEffect* gray_ = nullptr;
	VignetteEffect* vignette_ = nullptr;
};
