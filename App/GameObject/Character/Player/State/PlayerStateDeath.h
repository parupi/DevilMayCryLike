#pragma once 
#include "PlayerStateBase.h"
#include "Math/Vector3.h"

/// <summary>
/// 死亡状態。とどめを受けてから「やられた」と分かるまでを段取りで見せる。
///
///   Fall     … 吹き飛びながら死亡モーションで倒れる（クリップの長さいっぱい）
///   Linger   … 倒れたポーズのまま静止。画面から色が抜けて視界が閉じていく
///   Dissolve … 体と武器がディゾルブ＋黒いもやになって消える
///   Finished … 消えきった状態を保つ。ゲームオーバーの選択は GameScene 側が出す
///
/// 世界の時間（敵の動き）を落として止めるのは GameSceneStatePlay の役目で、
/// このステートは Player::Update が渡す実時間ベースの deltaTime で進む。
/// </summary>
class PlayerStateDeath : public PlayerStateBase
{
public:
	PlayerStateDeath();
	~PlayerStateDeath() override = default;
	void Enter(Player& player) override;
	void Update(Player& player, float deltaTime) override;
	void Exit(Player& player) override;
	void ExecuteCommand(Player& player, const PlayerCommand& command) override;
	const char* GetDebugName() const override { return "Death"; };

private:
	enum class Phase {
		Fall,     // 吹き飛び＋死亡モーション
		Linger,   // 倒れたまま静止
		Dissolve, // ディゾルブで消える
		Finished, // 消えきった
	};

	// 段階を進める（段階内のタイマーを 0 に戻す）
	void BeginPhase(Phase phase);
	// 死亡クリップの長さ[s]。まだ流れていなければ 0 を返す
	float ResolveMotionDuration(Player& player) const;

	Phase phase_ = Phase::Fall;
	float phaseTimer_ = 0.0f; // 今の段階の経過時間[s]
	float totalTimer_ = 0.0f; // 演出全体の経過時間[s]
	// 死亡クリップの長さ。Player::UpdateAnimation が流し始めてからでないと取れないので、
	// 最初のフレームでは 0 のまま（取れるまで毎フレーム試す）
	float motionDuration_ = 0.0f;

	// ── 調整パラメータ ──
	// 倒れるモーションは Alien_Death(2.29秒) を最後まで見せる。
	// クリップの長さが取れなかったとき（静的モデル）だけこの値を使う
	static constexpr float kFallbackMotionDuration = 2.3f;
	static constexpr float kLingerDuration = 0.35f;   // 倒れたまま見せる間
	static constexpr float kDissolveDuration = 0.9f;  // 溶けて消えるまで
	static constexpr float kHudFadeDuration = 0.7f;   // HUDが消えるまで
	// 吹き飛びの下限[m/s]。攻撃側の吹き飛ばしが弱くても、倒れたことが伝わるようにする
	static constexpr float kLaunchMinSpeed = 4.0f;
	static constexpr float kLaunchMinUpSpeed = 3.0f;
	static constexpr float kGravity = -12.0f;         // 他のステートと同じ落下加速度
	static constexpr float kHorizontalDamp = 2.5f;    // 着地後に滑り続けないよう水平を減衰させる
	// BGMを引くまでの時間。演出の頭で静かにする
	static constexpr float kBgmFadeTime = 1.0f;
};
