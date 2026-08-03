#pragma once
#include "GameObject/Effect/HitStop.h"
#include <Math/Vector2.h>
#include <cstddef>

class RadialBlurEffect;
class ChromaticAberrationEffect;
class HitFlashEffect;

/// <summary>
/// 攻撃がヒットした瞬間のポストエフェクト（放射状ブラー・色収差・フラッシュ）を
/// 1つのタイマーでまとめて駆動する。
/// 振れ幅と長さは攻撃の強さ（HitStopStrength）で切り替わるので、
/// ヒットストップ・カメラシェイクと同じ強さで演出が揃う。
/// </summary>
class HitPostEffect {
public:
	void Initialize();

	/// <param name="deltaTime">
	/// ヒットストップの影響を受けない実時間を渡すこと。
	/// スケール済みの時間を渡すと、時間停止中に演出が固まったままになる。
	/// </param>
	void Update(float deltaTime);

	/// <param name="screenUV">ヒット位置のスクリーンUV(0〜1)。画面外なら中央を渡す</param>
	void Play(HitStopStrength strength, const Vector2& screenUV);

#ifdef _DEBUG
	// 表示用。UIは App/Editor/Windows/HitEffectWindow.cpp にある。
	// エフェクト側の GetEffectData() が非constなので、ポインタも非constで返す
	bool IsPlaying() const { return isPlaying_; }
	RadialBlurEffect* GetRadialBlur() const { return radialBlur_; }
	ChromaticAberrationEffect* GetChroma() const { return chroma_; }
	HitFlashEffect* GetFlash() const { return flash_; }
#endif // _DEBUG

	void Stop();

private:
	// 強さごとの演出量
	struct Preset {
		float duration;       // 演出の長さ[秒]
		float blurStrength;   // 放射状ブラーの伸び
		float chromaStrength; // 色収差のずらし量
		float flashIntensity; // フラッシュの強さ
	};
	static const Preset kPresets[static_cast<size_t>(HitStopStrength::Count)];

	// 光の色（わずかに暖色にして金属の火花っぽく見せる）
	static constexpr float kFlashColorR = 1.0f;
	static constexpr float kFlashColorG = 0.95f;
	static constexpr float kFlashColorB = 0.85f;

	// eased: 1.0(ヒット直後) → 0.0(終了) の減衰値
	void Apply(float eased);

	RadialBlurEffect* radialBlur_ = nullptr;
	ChromaticAberrationEffect* chroma_ = nullptr;
	HitFlashEffect* flash_ = nullptr;

	Vector2 center_{ 0.5f, 0.5f };
	Preset current_{};
	float timer_ = 0.0f;
	bool isPlaying_ = false;
};
