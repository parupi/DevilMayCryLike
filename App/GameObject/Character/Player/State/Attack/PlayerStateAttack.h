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
	// 攻撃の名前を取得
	const std::string& GetAttackName() const { return name_; }
	// 先行入力バッファから発火したリクエストがあるか
	bool HasPendingRequest() const { return pendingRequest_.type != AttackRequest::None; }
	// 先行入力リクエストを取り出す（消費）
	AttackRequestData ConsumePendingRequest() { auto r = pendingRequest_; pendingRequest_ = {}; return r; }
private:
	// タイミングに基づいて次の攻撃リクエストを生成する
	AttackRequestData BuildRequestFromNode(Player& player);
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
	AttackRequestData pendingRequest_{};
};

