#pragma once
#include "GameObject/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"

enum class AttackType {
	Thrust, // 刺突
	Slash, // 斬撃
};

// 攻撃の情報
struct AttackData {
	int32_t pointCount;
	std::vector<Vector3> controlPoints;
};

class PlayerStateAttackBase : public PlayerStateBase
{
public:
	PlayerStateAttackBase(std::string attackName);
	virtual ~PlayerStateAttackBase() = default;
	virtual void Enter(Player& player) = 0;
	virtual void Update(Player& player) = 0;
	virtual void Exit(Player& player) = 0;

	AttackData GetAttackData() { return attackData_; }

	// 制御点の更新
	void UpdateControlPoints();
	// 制御点描画
	void DrawControlPoints();
	// 攻撃の名前
	std::string name;
protected:
	


	GlobalVariables* gv = GlobalVariables::GetInstance();

	AttackData attackData_;
};

