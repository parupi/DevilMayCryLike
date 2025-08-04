#pragma once
#include "GameObject/Player/State/PlayerStateBase.h"
#include "debuger/GlobalVariables.h"
#include "3d/Object/Object3d.h"

enum class AttackType {
	Thrust, // 刺突
	Slash, // 斬撃
};

enum class AttackPosture {
	Stand, // 立ち状態
	Air, // 空中状態
};

// 攻撃の情報
struct AttackData {
	// 挙動系
	int32_t pointCount;                      // 制御点の数
	std::vector<Vector3> controlPoints;      // 制御点（Vector3）

	// 移動系
	Vector3 moveVelocity{};                  // 攻撃中の移動速度
	Vector3 knockBackSpeed{};             // 敵のノックバック速度

	// タイマー系
	float totalDuration = 0.0f;              // 攻撃全体にかかる時間
	float preDelay = 0.0f;                   // 予備動作の時間
	float attackDuration = 0.0f;             // 攻撃中の時間
	float postDelay = 0.0f;                  // 技後の硬直時間
	float nextAttackDelay = 0.0f;            // 次のアクションを受け付ける時間

	// その他
	bool drawDebugControlPoints = false;     // 制御点のデバッグ描画フラグ

	// 攻撃力
	float damage = 0.0f;                      // ダメージ値

	// 派生先
	AttackPosture posture = AttackPosture::Stand;
};

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

