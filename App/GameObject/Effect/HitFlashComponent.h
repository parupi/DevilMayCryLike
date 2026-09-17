#pragma once
#include <vector>
#include "Math/Vector4.h"
#include "Graphics/Rendering/Particle/Curve.h"

class BaseRenderer;

/// <summary>
/// 被弾したモデルを一瞬白く光らせるコンポーネント（設計書 §27 Hit Flash）。
///
/// `BaseRenderer::SetEmissiveTint`（rgb=加算発光色 / a=強度）を使うため、
/// シェーダーやマテリアルの改修は不要。
///
/// 他システム（ボスのスーパーアーマー発光など）も同じ EmissiveTint を毎フレーム
/// 書き込んでいるので、以下の作法で衝突を避けている。
///  - Start() 時点のティントを覚えておき、フラッシュ終了時にそれへ戻す
///  - フラッシュ中でも、相手がより強く光らせているフレームは上書きしない
///  - 所有者の Update より後に更新すること（先に呼ぶと相手に消される）
/// </summary>
class HitFlashComponent
{
public:
	HitFlashComponent();

	/// <summary>フラッシュ対象のレンダラーを追加する（本体・武器など）</summary>
	void AddRenderer(BaseRenderer* renderer);

	/// <summary>フラッシュを開始する</summary>
	/// <param name="duration">元に戻るまでの時間[秒]</param>
	/// <param name="intensity">発光の強さ。EmissiveTint の a に入る</param>
	void Start(float duration = kDefaultDuration, float intensity = kDefaultIntensity);

	/// <summary>所有者の見た目更新より後に呼ぶこと</summary>
	void Update(float deltaTime);

	/// <summary>フラッシュを即座に打ち切って元のティントへ戻す（死亡演出の開始時など）</summary>
	void Stop();

	bool IsPlaying() const { return isPlaying_; }

private:
	static constexpr float kDefaultDuration = 0.09f;
	static constexpr float kDefaultIntensity = 2.5f;
	// フラッシュの色。金属の火花に寄せてわずかに暖色にしている
	static constexpr float kFlashR = 1.0f;
	static constexpr float kFlashG = 0.97f;
	static constexpr float kFlashB = 0.92f;

	struct Target {
		BaseRenderer* renderer = nullptr;
		Vector4 savedTint{};  // Start時のティント。終了時にここへ戻す
	};

	// 元のティントを覚えて全対象へ書き戻す
	void RestoreAll();

	std::vector<Target> targets_;

	// 1.0 → 0.0 の減衰カーブ。最初に強く落ちるほど「弾かれた」感じが出る
	Curve decayCurve_;

	float timer_ = 0.0f;
	float duration_ = kDefaultDuration;
	float intensity_ = kDefaultIntensity;
	bool isPlaying_ = false;
};
