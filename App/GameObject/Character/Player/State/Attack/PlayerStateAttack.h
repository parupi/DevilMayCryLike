#pragma once
#include "GameObject/Character/Player/State/PlayerStateBase.h"
#include "GameObject/Character/Player/Controller/PlayerInput.h"
#include "Debugger/GlobalVariables.h"
#include "World3D/Object/Object3d.h"
#include "GameObject/Character/CharacterStructs.h"
#include "AttackStructs.h"
class Player;

class PlayerStateAttack
{
public:
	PlayerStateAttack(std::string attackName);
	~PlayerStateAttack() = default;
	void Enter(Player& player);
	void  Update(Player& player, float deltaTime);
	void Exit(Player& player);
	// 入力に基づいて次の行動を通知
	AttackRequestData ExecuteCommand(Player& player, const PlayerCommand& command);
	// 割り込みされたタイミングの処理
	void OnInterrupted(Player& player);

	AttackData GetAttackData() { return attackData_; }

	// 制御点の更新
	void UpdateAttackData();
	// 制御点描画
	void DrawControlPoints(Player& player);
	// 攻撃の名前
	std::string name_;
	// 攻撃が終了したかどうか
	bool IsFinished() const { return isFinish_; };
	// 中断できるかどうか
	bool CanBeInterrupted() const { return attackPhase_ == AttackPhase::Cancel; }
	// 派生先の攻撃があるかどうか
	bool HasBranch(Player& player) const;
	// 派生先のどれかをこのボタンで出せるか
	bool CanBranchWith(Player& player, InputButton button) const;
	// 攻撃の名前を取得
	const std::string& GetAttackName() const { return name_; }
	// 先行入力バッファから発火したリクエストがあるか
	bool HasPendingRequest() const { return pendingRequest_.type != AttackRequest::None; }
	// 先行入力リクエストを取り出す（消費）
	AttackRequestData ConsumePendingRequest() { auto r = pendingRequest_; pendingRequest_ = {}; return r; }
	// 溜め攻撃の溜め中か（構えで止まって、ボタンを離すのを待っている）
	bool IsCharging() const { return attackPhase_ == AttackPhase::Charge; }

	// ── 演出（PlayerAttackEffect）が見る状態。攻撃の中身は変えない ──
	// 予備動作（構え）の途中か
	bool IsStartupPhase() const { return attackPhase_ == AttackPhase::Startup; }
	// 攻撃判定が出ている（剣を振っている）間か
	bool IsActivePhase() const { return attackPhase_ == AttackPhase::Active; }
	/// <summary>
	/// 剣を制御点で動かしている間か（構え〜振り抜き）。
	/// 硬直・派生待ちでは制御点が剣を動かさないので、その間は手へ戻してよい
	/// </summary>
	bool IsSwingPhase() const {
		return attackPhase_ == AttackPhase::Startup
			|| attackPhase_ == AttackPhase::Charge
			|| attackPhase_ == AttackPhase::Active;
	}
	/// <summary>
	/// 構え＋振りを 0〜1 にした進み具合。攻撃ごとのボーン補正の出入りに使う。
	/// 硬直・派生待ちは含めない（そこまで補正を引っぱると次の技に被る）
	/// </summary>
	float GetMotionProgress() const {
		const float span = attackData_.preDelay + attackData_.attackDuration;
		if (span <= 0.0f) return 1.0f;
		const float t = stateTime_.current / span;
		return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	}
	// 予備動作の進み具合 0〜1
	float GetStartupProgress() const {
		if (attackData_.preDelay <= 0.0f) return 1.0f;
		const float t = stateTime_.current / attackData_.preDelay;
		return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	}
	// 振りの進み具合 0〜1
	float GetActiveProgress() const {
		if (attackData_.attackDuration <= 0.0f) return 1.0f;
		const float t = (stateTime_.current - attackData_.preDelay) / attackData_.attackDuration;
		return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	}
	// 溜め具合 0〜1。溜めている間は「今離したらこうなる」値、振り始めた後は確定した値
	float GetChargeProgress() const {
		if (!attackData_.isCharge) return 0.0f;
		if (isChargeReleased_) return chargeRatio_;
		const float range = attackData_.chargeMaxTime - attackData_.chargeMinTime;
		if (range <= 0.0f) return (chargeTime_ >= attackData_.chargeMaxTime) ? 1.0f : 0.0f;
		const float t = (chargeTime_ - attackData_.chargeMinTime) / range;
		return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
	}
	// エディタの値そのままの攻撃データ（溜め・最終段の差し替え前）
	const AttackData& GetBaseAttackData() const { return attackData_; }
private:
	// タイミングに基づいて次の攻撃リクエストを生成する。
	// 押したボタンで出せる派生先だけを候補にし、その中から入力のタイミングで選ぶ
	AttackRequestData BuildRequestFromNode(Player& player, InputButton button);
	// 溜めの更新。溜めている間は true を返し、攻撃の時間を進めない
	bool UpdateCharge(Player& player, float deltaTime);
	// 溜めを終えて振り始める。溜めた時間から倍率を決める
	void ReleaseCharge(Player& player);
	// 溜め中のループ音を止める。鳴っていなければ何もしない
	void StopChargeSound();
	// この攻撃に割り当てたボタンを押し続けているか
	bool IsAttackButtonHeld(Player& player) const;
	// 多段ヒットの段を進める。段が変わるたびに当たり直させる
	void UpdateHitSegment(Player& player);
	// ノックバック性能を GlobalVariables から読む（仕様書 §3）。
	// prefix は "" で通常、"Final" で最終段ぶん。
	// 時間・減速・向き・合成方法は段で変える必要が無いので、最終段でも共通の値を読む
	KnockbackData LoadKnockback(const std::string& prefix) const;
	// 最終段・溜めの倍率を反映した攻撃データを Player へ渡す（敵はこれを見てダメージを受ける）
	void ApplyHitData(Player& player);
	// フェーズの更新
	void UpdatePhase(float time);
	// 予備動作の更新
	void UpdateStartup(Player& player, float deltaTime);
	// アクティブ状態の更新
	void UpdateActive(Player& player);
	// 後隙状態の更新
	void UpdateRecovery(Player& player);

	TimeData stateTime_{};

	enum class AttackPhase {
		Startup, // 予備動作
		Charge, // 溜め（溜め攻撃だけ。構えたまま止まる）
		Active, // 攻撃中
		Recovery, // 硬直
		Cancel, // 入力待ち時間
	}attackPhase_{};

	GlobalVariables* gv = &GlobalVariables::GetInstance();

	AttackData attackData_{};
	// 派生先を管理するためのタイマー
	TimeData attackChangeTimer_{};

	bool isFinish_ = false;
	// 先行入力バッファ
	bool hasPendingBuffer_ = false;
	// 先行入力で押されたボタン。Cancel フェーズに入ったときに派生先のボタンと照らし合わせる
	InputButton pendingButton_ = InputButton::None;
	AttackRequestData pendingRequest_{};

	// ── 溜め ──
	// 溜めた時間[秒]
	float chargeTime_ = 0.0f;
	// 溜め具合 0〜1（Charge Min Time で 0、Charge Max Time で 1）
	float chargeRatio_ = 0.0f;
	// 溜めを終えて振り始めたか
	bool isChargeReleased_ = false;
	// 溜めきったことを知らせたか
	bool isChargeFullNotified_ = false;
	// 溜め中のループ音の再生番号。-1 なら鳴っていない
	int chargeVoice_ = -1;

	// ── 多段ヒット ──
	// 今が何段目か（0 始まり）
	int32_t hitIndex_ = 0;
};

