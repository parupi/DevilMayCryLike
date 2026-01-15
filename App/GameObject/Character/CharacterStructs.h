#pragma once
#include <math/Vector3.h>
#include <vector>

enum class AttackType {
	Thrust, // 刺突
	Slash, // 斬撃
};

enum class AttackPosture {
	Stand, // 立ち状態
	Air, // 空中状態
};

enum class ReactionType {
	HitStun,   // のけぞり
	Knockback, // 吹っ飛び
	Launch     // 打ち上げ
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

	// HitStop
	float hitStopTime;
	float hitStopIntensity;

	// 攻撃を受けた側に送る情報
	ReactionType type = ReactionType::HitStun;

	// ノックバック＆打ち上げ共通
	float impulseForce = 0.0f;
	float upwardRatio = 0.0f;    // Launchはここを高めに

	// 吹っ飛び用
	float torqueForce = 0.0f;

	// のけぞり用
	float stunTime = 0.0f;
};



struct DamageInfo {
    float damage = 0.0f;

    Vector3 hitPosition;
    Vector3 hitNormal;
    Vector3 attackerPosition;
    // ノックバックの方向
    Vector3 direction;

    ReactionType type = ReactionType::HitStun;

    // ノックバック＆打ち上げ共通
    float impulseForce = 0.0f;
    float upwardRatio = 0.0f;    // Launchはここを高めに

    // 吹っ飛び用
    float torqueForce = 0.0f;

    // のけぞり用
    float stunTime = 0.0f;
};

struct MoveIntent {
	Vector3 moveDir; // 入力方向
	float moveScale; // 0.0f〜1.0f
	bool jump; // このフレームでジャンプしたい
	bool dash; // ダッシュしたい
};