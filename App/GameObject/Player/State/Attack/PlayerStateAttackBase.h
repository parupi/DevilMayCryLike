#pragma once
#include "GameObject/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"

enum class AttackType {
	Thrust, // 刺突
	Slash, // 斬撃
};

// 攻撃の情報
struct AttackData {
	// 挙動系
	int32_t pointCount;                      // 制御点の数
	std::vector<Vector3> controlPoints;      // 制御点（Vector3）

	// 移動系
	float moveSpeed = 0.0f;                  // 攻撃中の移動速度
	float knockBackSpeed = 0.0f;             // 敵のノックバック速度

	// タイマー系
	float totalDuration = 0.0f;              // 攻撃全体にかかる時間
	float preDelay = 0.0f;                   // 予備動作の時間
	float attackDuration = 0.0f;             // 攻撃中の時間
	float postDelay = 0.0f;                  // 技後の硬直時間
	float nextAttackDelay = 0.0f;            // 次の攻撃が可能になるまでの時間

	// その他
	bool drawDebugControlPoints = false;     // 制御点のデバッグ描画フラグ

	// 攻撃力
	float damage = 0.0f;                      // ダメージ値
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
	void UpdateAttackData();
	// 制御点描画
	void DrawControlPoints(Player& player);
	// 攻撃の名前
	std::string name_;
protected:
	


	GlobalVariables* gv = GlobalVariables::GetInstance();

	AttackData attackData_;
};

