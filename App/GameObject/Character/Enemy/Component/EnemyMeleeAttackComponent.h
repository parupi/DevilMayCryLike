#pragma once
#include <Math/Vector3.h>
#include <vector>
#include "GameObject/Effect/AttackTelegraph.h"

class Enemy;
class Object3d;

struct MeleeAttackParams {
	float windupDuration = 0.5f;   // 予備動作: 構えポーズを保持する時間
	float attackDuration = 0.25f;  // 攻撃モーション: CatmullRom スイングの時間
	float rushSpeed = 0.0f;   // > 0 なら攻撃フェーズ中プレイヤーへ突進

	// weaponTranslate[1] と weaponRotate[1] が構えポーズ（t=0）になる
	std::vector<Vector3> weaponTranslate;
	std::vector<Vector3> weaponRotate;

	// 地面に出す予兆（赤いマーカー）。shape が None なら出さない。
	// 予備動作の間だけ出して、振り始める瞬間に塗りが外枠へ届く。
	// 大きさは **オブジェクトのスケール1 のときのワールド単位** で書く
	AttackTelegraphParams telegraph;
};

/// <summary>
/// 近接攻撃実行コンポーネント。
/// 予備動作（Windup）フェーズで構えを保持した後、攻撃フェーズでスイングする。
/// </summary>
class EnemyMeleeAttackComponent {
public:
	explicit EnemyMeleeAttackComponent(Object3d* weapon);

	void BeginAttack(Enemy& enemy, const MeleeAttackParams& params);
	void Update(Enemy& enemy, float deltaTime);

	/// <summary>
	/// 予兆マーカーを消す。**必ずステートの Exit() から呼ぶこと**。
	/// 被弾などで予備動作の途中にステートが切り替わると Update が回らなくなり、
	/// マーカー側が「攻撃が発生した」と誤認して閃光を出してしまう
	/// </summary>
	void CancelTelegraph();

	bool IsFinished()  const { return finished_; }
	bool IsWindingUp() const { return !finished_ && timer_ < params_.windupDuration; }

private:
	void ApplyWeaponPose(float t);
	// 予兆マーカーを敵の足元へ出し直す。振り始めたら呼ぶのをやめて自動で畳ませる
	void UpdateTelegraph(Enemy& enemy);

	Object3d* weapon_;
	MeleeAttackParams params_;
	float timer_ = 0.0f;
	bool finished_ = true;
};
