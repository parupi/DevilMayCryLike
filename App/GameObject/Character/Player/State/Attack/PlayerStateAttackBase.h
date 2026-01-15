#pragma once
#include "GameObject/Character/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"
#include "3d/Object/Object3d.h"
#include "GameObject/Character/CharacterStructs.h"
#include "AttackStructs.h"

class PlayerStateAttackBase : public PlayerStateBase
{
private:

public:
	PlayerStateAttackBase(std::string attackName);
	virtual ~PlayerStateAttackBase() = default;
	virtual void Enter(Player& player);
	virtual void Update(Player& player);
	virtual void Exit(Player& player);

	AttackData GetAttackData() { return attackData_; }

	// 制御点の更新
	void UpdateAttackData();
	// 制御点描画
	void DrawControlPoints(Player& player);
	// 攻撃の名前
	std::string name_;

	bool GetCurrentInput(InputType& outInput, StickDir& outDir);
protected:
	
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
};

