#pragma once
#include "GameObject/Character/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"
#include "3d/Object/Object3d.h"
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

	AttackData GetAttackData() { return attackData_; }

	// 制御点の更新
	void UpdateAttackData();
	// 制御点描画
	void DrawControlPoints(Player& player);
	// 攻撃の名前
	std::string name_;

	bool GetCurrentInput(InputType& outInput, StickDir& outDir);

	bool IsFinished() const { return isFinish_; };
	// 中断できるかどうか
	bool CanBeInterrupted() const { return attackPhase_ == AttackPhase::Cancel; }
private:
	// フェーズの更新
	void UpdatePhase(float time);
	// 予備動作の更新
	void UpdateStartup(Player& player);
	// アクティブ状態の更新
	void UpdateActive(Player& player);
	// 後隙状態の更新
	void UpdateRecovery(Player& player);
	// 入力受付状態の更新
	AttackRequestData UpdateCancel(Player& player);
	
	TimeData stateTime_;

	enum class AttackPhase {
		Startup, // 予備動作
		Active, // 攻撃中
		Recovery, // 硬直
		Cancel, // 入力待ち時間
	}attackPhase_;

	GlobalVariables* gv = GlobalVariables::GetInstance();

	AttackData attackData_;
	// 派生先を管理するためのタイマー
	TimeData attackChangeTimer_;

	std::vector<AttackBranch> branches_;   // 派生定義（UI & 判定共通）
	bool isFinish_ = false;
};

