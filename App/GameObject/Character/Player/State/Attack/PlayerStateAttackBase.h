#pragma once
#include "GameObject/Character/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"
#include "3d/Object/Object3d.h"
#include "GameObject/Character/CharacterStructs.h"

class PlayerStateAttackBase : public PlayerStateBase
{
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
};

