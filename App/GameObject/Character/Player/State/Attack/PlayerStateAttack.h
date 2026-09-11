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
private:
	// タイミングに基づいて次の攻撃リクエストを生成する。
	// 押したボタンで出せる派生先だけを候補にし、その中から入力のタイミングで選ぶ
	AttackRequestData BuildRequestFromNode(Player& player, InputButton button);
	// 溜めの更新。溜めている間は true を返し、攻撃の時間を進めない
	bool UpdateCharge(Player& player, float deltaTime);
	// 溜めを終えて振り始める。溜めた時間から倍率を決める
	void ReleaseCharge(Player& player);
	// この攻撃に割り当てたボタンを押し続けているか
	bool IsAttackButtonHeld(Player& player) const;
	// 多段ヒットの段を進める。段が変わるたびに当たり直させる
	void UpdateHitSegment(Player& player);
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

	// ── 多段ヒット ──
	// 今が何段目か（0 始まり）
	int32_t hitIndex_ = 0;
};

