#pragma once
#include <Math/Vector3.h>
#include <vector>
#include "EnemyAttackAim.h"

class Enemy;
class Object3d;

struct MeleeAttackParams {
	float windupDuration = 0.5f;   // 予備動作: 構えポーズを保持する時間
	float attackDuration = 0.25f;  // 攻撃モーション: CatmullRom スイングの時間
	float rushSpeed = 0.0f;   // > 0 なら攻撃フェーズ中、振り始めに向いていた正面へまっすぐ突進

	// weaponTranslate[1] と weaponRotate[1] が構えポーズ（t=0）になる
	std::vector<Vector3> weaponTranslate;
	std::vector<Vector3> weaponRotate;

	// 地面に出す予兆（赤いマーカー）。shape が None なら出さない。
	// 予備動作の間だけ出して、振り始める瞬間に塗りが外枠へ届く。
	// 振り始めるとこの向き・場所で体が固定されるので、攻撃は予兆の上をなぞって出る
	// （当たり判定そのものは武器のコライダーが持つ）。
	// 大きさは **オブジェクトのスケール1 のときのワールド単位** で書く
	AttackTelegraphParams telegraph;

	// 攻撃の音。null なら鳴らさない。
	// ステート側に書かず params に持たせているのは、予兆や武器の軌道と同じ場所で
	// 1つの攻撃の性格をまとめて見られるようにするため
	const char* windupSound = nullptr;  // 構えに入った瞬間
	const char* swingSound = nullptr;   // 振り始めた瞬間
	float soundVolume = 0.8f;
};

/// <summary>
/// 近接攻撃実行コンポーネント。
/// 予備動作（Windup）フェーズで構えを保持した後、攻撃フェーズでスイングする。
/// 振り始めの瞬間に体の向き・突進の進路を予兆の場所で固定し、以降はプレイヤーを追わない（EnemyAttackAim）。
/// </summary>
class EnemyMeleeAttackComponent {
public:
	explicit EnemyMeleeAttackComponent(Object3d* weapon);

	void BeginAttack(Enemy& enemy, const MeleeAttackParams& params);
	void Update(Enemy& enemy, float deltaTime);

	/// <summary>
	/// 予兆と向きの固定を後始末する。**必ずステートの Exit() から呼ぶこと**。
	/// 被弾などで予備動作の途中にステートが切り替わると Update が回らなくなり、
	/// マーカー側が「攻撃が発生した」と誤認して閃光を出してしまう。
	/// 振り始めた後に中断された場合は、体の向きの固定が戻らず振り向かなくなる
	/// </summary>
	void Cancel(Enemy& enemy);

	bool IsFinished()  const { return finished_; }
	bool IsWindingUp() const { return !finished_ && timer_ < params_.windupDuration; }

private:
	void ApplyWeaponPose(float t);

	Object3d* weapon_;
	MeleeAttackParams params_;
	// 予兆と、振り始めに固定する狙い（体の向き・突進の進路）
	EnemyAttackAim aim_;
	float timer_ = 0.0f;
	bool finished_ = true;
	// 振りの音は1回だけ。Attack フェーズは毎フレーム通るので、鳴らしたかを覚えておく
	bool swingSoundPlayed_ = false;
};
