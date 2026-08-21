#pragma once
#include <vector>
#include <Math/Vector4.h>

class Object3d;
class BaseRenderer;

/// <summary>
/// 敵の出現・死亡演出。
/// - 出現: 黒い粒子が出現位置へ収束しながら、ディゾルブで実体化する
/// - 死亡: 吹き飛び → 死亡モーション → 黒いもや → ディゾルブ の順に見せる（DeathStage）
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

	/// <summary>
	/// 死亡演出の段取り。Dying の中をこの順に進める。
	/// 消え始めるのは最後の Dissolve だけで、それまでは実体のまま「倒れた」ことを見せる。
	/// </summary>
	enum class DeathStage {
		Launch,   // 吹き飛び（実体のまま飛ぶ。粒子もディゾルブもまだ出さない）
		Motion,   // 死亡モーション（着地して崩れ落ちる。終わり際からもやが漏れ始める）
		Smoke,    // 黒いもやが体から立ち上る
		Dissolve, // もやを出しながらディゾルブで消えていく
	};

	void Initialize(Object3d* owner);

	/// <summary>ディゾルブ対象のレンダラーを追加する（本体・武器など）</summary>
	void AddRenderer(BaseRenderer* renderer);

	/// <summary>出現演出を開始する（粒子収束 + ディゾルブイン）</summary>
	void StartAppear();

	/// <summary>出現演出の長さ[s]を設定する（ボス出現イベントなどでゆっくり出したいとき用）</summary>
	void SetAppearDuration(float seconds) { appearDuration_ = seconds; }
	/// <summary>出現演出の長さ[s]。出現モーションをこの尺に合わせるのに使う</summary>
	float GetAppearDuration() const { return appearDuration_; }

	/// <summary>収束粒子の1回あたり発生数を設定する（ボスなど大型の敵で濃くしたいとき用）</summary>
	void SetAppearEmitCount(int count) { appearEmitCount_ = count; }

	/// <summary>死亡演出を開始する（吹き飛び → 死亡モーション → 黒いもや → ディゾルブ）</summary>
	void StartDeath();

	/// <summary>
	/// 死亡クリップを流す尺[s]（吹き飛び + 死亡モーション）。
	/// これより後（もや・ディゾルブ）はクリップの最後のポーズで倒れたままになる
	/// </summary>
	float GetDeathClipDuration() const { return kDeathLaunchDuration + kDeathMotionDuration; }

	/// <summary>死亡演出全体の長さ[s]</summary>
	float GetDeathTotalDuration() const {
		return kDeathLaunchDuration + kDeathMotionDuration + kDeathSmokeDuration + kDeathDissolveDuration;
	}

	DeathStage GetDeathStage() const { return deathStage_; }

	void Update(float deltaTime);

	bool IsAppearing() const { return phase_ == Phase::Appearing; }
	bool IsDying() const { return phase_ == Phase::Dying; }
	bool IsDeathFinished() const { return phase_ == Phase::DeathFinished; }
	/// <summary>出現または死亡の演出再生中か</summary>
	bool IsPlaying() const { return phase_ == Phase::Appearing || phase_ == Phase::Dying; }

private:
	// 死亡演出の各段階を進める
	void UpdateDeath(float deltaTime);
	// 次の段階へ移る（段階内のタイマーを 0 に戻す）
	void BeginDeathStage(DeathStage stage);
	// 発生間隔タイマーを進めて、このフレームで発生させる回数を返す
	int ConsumeEmitTicks(float deltaTime);

	// 全レンダラーにディゾルブ量を適用する
	void ApplyDissolve(float threshold, const Vector4& edgeColor);
	// ディゾルブ上書きを解除する（マテリアル設定に戻す）
	void ClearDissolve();
	// パーティクルを発生させる（グループ未登録のシーンでは何もしない）
	void EmitParticle(const char* groupName, int count);

	Object3d* owner_ = nullptr;
	std::vector<BaseRenderer*> renderers_;

	Phase phase_ = Phase::None;
	DeathStage deathStage_ = DeathStage::Launch;
	float timer_ = 0.0f;      // 出現演出の経過時間[s]
	float stageTimer_ = 0.0f; // 死亡演出の「今の段階」の経過時間[s]
	float emitTimer_ = 0.0f;

	// ── 調整パラメータ ──
	float appearDuration_ = 1.2f;                  // 出現演出の長さ[s]（Setterで変更可）
	int appearEmitCount_ = 3;                      // 収束粒子の1回あたり発生数（Setterで変更可）

	// 死亡演出の各段階の長さ[s]。合計がやられてから消えるまでの時間になる
	static constexpr float kDeathLaunchDuration = 0.35f;
	static constexpr float kDeathMotionDuration = 0.80f;
	static constexpr float kDeathSmokeDuration = 0.50f;
	static constexpr float kDeathDissolveDuration = 0.85f;
	// 死亡モーションの終わり何秒前からもやが漏れ始めるか（段階の切り替わりを繋ぐ）
	static constexpr float kSmokeLeadTime = 0.25f;

	static constexpr float kEmitInterval = 0.05f;  // 粒子の発生間隔[s]
	static constexpr int kDeathEmitCount = 3;      // ディゾルブ中に散る粒子の1回あたり発生数
	static constexpr int kSmokeLeadCount = 1;      // 漏れ始めのもやの1回あたり発生数
	static constexpr int kSmokeBurstCount = 10;    // もや段階の頭で一気に噴き出す数
	static constexpr int kSmokeEmitCount = 2;      // もや段階の1回あたり発生数
	static constexpr int kDissolveSmokeCount = 1;  // ディゾルブ中も薄く出し続けるもやの数
	static constexpr float kEdgeWidth = 0.08f;     // ディゾルブ縁の太さ

	// 立ち上る黒いもや。ディゾルブ中に散る粒子（EnemyDeathParticle）とは別グループ。
	// プレイヤーの死亡演出（DissolveOutEffect）とも共有している
	static constexpr const char* kSmokeGroup = "DeathSmoke";
	static constexpr const char* kScatterGroup = "EnemyDeathParticle";
	static constexpr const char* kSpawnGroup = "EnemySpawnParticle";

	// ディゾルブ縁の発光色（rgb + 強度）
	static constexpr Vector4 kAppearEdgeColor = { 0.85f, 0.1f, 0.1f, 6.0f };
	static constexpr Vector4 kDeathEdgeColor = { 0.9f, 0.15f, 0.05f, 8.0f };
};
