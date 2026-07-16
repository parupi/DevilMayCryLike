#pragma once
#include <vector>
#include <Math/Vector4.h>

class Object3d;
class BaseRenderer;

/// <summary>
/// 敵の出現・死亡演出。
/// - 出現: 黒い粒子が出現位置へ収束しながら、ディゾルブで実体化する
/// - 死亡: 敵の位置から黒い粒子を撒き散らしながら、ディゾルブで消滅する
/// ディゾルブはレンダラー単位の上書き（BaseRenderer::SetDissolveThreshold）を使うため、
/// 同じモデルを使う他の敵には影響しない。
/// </summary>
class EnemyAppearanceEffect {
public:
	enum class Phase {
		None,          // 演出なし（通常状態）
		Appearing,     // 出現演出中
		Dying,         // 死亡演出中
		DeathFinished, // 死亡演出終了（所有者はこれを見て isAlive を落とす）
	};

	void Initialize(Object3d* owner);

	/// <summary>ディゾルブ対象のレンダラーを追加する（本体・武器など）</summary>
	void AddRenderer(BaseRenderer* renderer);

	/// <summary>出現演出を開始する（粒子収束 + ディゾルブイン）</summary>
	void StartAppear();

	/// <summary>死亡演出を開始する（粒子拡散 + ディゾルブアウト）</summary>
	void StartDeath();

	void Update(float deltaTime);

	bool IsAppearing() const { return phase_ == Phase::Appearing; }
	bool IsDying() const { return phase_ == Phase::Dying; }
	bool IsDeathFinished() const { return phase_ == Phase::DeathFinished; }
	/// <summary>出現または死亡の演出再生中か</summary>
	bool IsPlaying() const { return phase_ == Phase::Appearing || phase_ == Phase::Dying; }

private:
	// 全レンダラーにディゾルブ量を適用する
	void ApplyDissolve(float threshold, const Vector4& edgeColor);
	// ディゾルブ上書きを解除する（マテリアル設定に戻す）
	void ClearDissolve();
	// パーティクルを発生させる（グループ未登録のシーンでは何もしない）
	void EmitParticle(const char* groupName, int count);

	Object3d* owner_ = nullptr;
	std::vector<BaseRenderer*> renderers_;

	Phase phase_ = Phase::None;
	float timer_ = 0.0f;
	float emitTimer_ = 0.0f;

	// ── 調整パラメータ ──
	static constexpr float kAppearDuration = 1.2f; // 出現演出の長さ[s]
	static constexpr float kDeathDuration = 0.9f;  // 死亡演出の長さ[s]
	static constexpr float kEmitInterval = 0.05f;  // 粒子の発生間隔[s]
	static constexpr int kAppearEmitCount = 3;     // 収束粒子の1回あたり発生数
	static constexpr int kDeathEmitCount = 3;      // 拡散粒子の1回あたり発生数
	static constexpr int kDeathBurstCount = 24;    // 死亡直後に一気に撒く粒子数
	static constexpr float kEdgeWidth = 0.08f;     // ディゾルブ縁の太さ

	// ディゾルブ縁の発光色（rgb + 強度）
	static constexpr Vector4 kAppearEdgeColor = { 0.85f, 0.1f, 0.1f, 6.0f };
	static constexpr Vector4 kDeathEdgeColor = { 0.9f, 0.15f, 0.05f, 8.0f };
};
