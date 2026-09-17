#pragma once
#include "Math/Vector3.h"

class HitFlashEffect;

/// <summary>
/// ジャスト回避成功時の演出を1か所にまとめたクラス。
///
/// 内訳は「白フラッシュ（ポストエフェクト）／円形の衝撃波（VFX）／専用SE／カメラシェイク」。
/// スローモーションだけは世界全体の時間に関わるので Player が持っている。
///
/// 攻撃ヒット時の HitPostEffect とは意図的に別のパスにしてある。
/// あちらは暖色の弱いフラッシュで「当てた」を伝えるもので、
/// こちらは純白の強いフラッシュで「避けた」を伝えるもの。同じ物にすると差が出ない。
/// </summary>
class JustDodgeEffect {
public:
	/// 再生する衝撃波のVFX名（Resource/VFX/JustDodgeBurst.vfx.json）
	static constexpr const char* kVFXName = "JustDodgeBurst";
	/// 専用SE。金属を弾いたような高音を想定（Resource/Sound/JustDodge.wav）
	static constexpr const char* kSEName = "JustDodge";

	/// ポストエフェクトのパスを登録する（Player::Initialize から）
	void Initialize();

	/// <param name="deltaTime">
	/// スローモーションの影響を受けない実時間を渡すこと。
	/// スケール済みの時間を渡すと、スロー中にフラッシュが焼き付いたまま止まる
	/// </param>
	void Update(float deltaTime);

	/// <summary>
	/// 演出を発生させる。
	/// </summary>
	/// <param name="position">プレイヤーの位置（衝撃波の中心）</param>
	/// <param name="attackDirection">攻撃してきた方向（衝撃波の向き）</param>
	/// <param name="shakeTrauma">カメラに加えるトラウマ量</param>
	void Play(const Vector3& position, const Vector3& attackDirection, float shakeTrauma);

	/// 演出を打ち切る（シーン切り替え・死亡時など）
	void Stop();

	bool IsPlaying() const { return isPlaying_; }

private:
	// フラッシュの長さ[秒]。長いと画面が白いままになるので短く切る
	static constexpr float kFlashDuration = 0.16f;
	// フラッシュの強さ。純白（rgb=1）で出す
	static constexpr float kFlashIntensity = 0.55f;

	HitFlashEffect* flash_ = nullptr;
	float timer_ = 0.0f;
	bool isPlaying_ = false;
};
