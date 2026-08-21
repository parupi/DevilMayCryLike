#pragma once
#include <string>
#include <Math/Vector3.h>
#include "GameObject/Character/CharacterStructs.h"

class Enemy;
class EnemyHitbox;

/// <summary>
/// 「体そのもので殴る」攻撃のパラメータ。武器を振る攻撃（MeleeAttackParams）の置き換え。
/// 判定の位置はアニメーションのポーズが決めるので、制御点は持たない。
/// </summary>
struct BoneAttackParams {
	std::string jointName = "Head";     // 判定を付けるジョイント
	// 判定の大きさ・オフセットは **オブジェクトのスケール1 のときのワールド単位** で書く。
	// BeginAttack が実際の配置スケールを掛けるので、ステージで敵を大きく置いても
	// 判定が置いていかれない（プレイヤーのコライダーは1辺1.0）
	Vector3 halfExtents{ 0.5f, 0.5f, 0.5f }; // 判定の大きさ
	Vector3 offset{};                    // ジョイント基準のオフセット
	DamageInfo damage;                   // この攻撃のダメージ・リアクション

	float duration = 1.0f;               // 攻撃全体の長さ[s]（クリップ長に合わせる）
	float rushSpeed = 0.0f;              // > 0 なら判定が出ている間プレイヤーへ突進

	// アニメーションイベント（hit_start / hit_end）が無いクリップ用のフォールバック。
	// 攻撃全体を 0〜1 としたときの判定ON/OFFの位置
	float hitStartRatio = 0.45f;
	float hitEndRatio = 0.70f;

	// イベントより下の比率を優先するか。
	// イベントはクリップ単位なので、同じクリップを使い回す攻撃（噛みつきと突進は
	// どちらも Dragon_Attack）で別々の判定時間にしたいときは false にする
	bool preferEvents = true;
};

/// <summary>
/// ボーン追従のヒットボックスで殴る攻撃を実行するコンポーネント。
///
/// クリップは等速で流し、判定のON/OFFは **アニメーションイベント** で取る
/// （Animation ウィンドウで hit_start / hit_end を打って .anim.json に保存する）。
/// イベントが無いクリップでは duration に対する比率で代用するので、
/// イベントを打つ前でも攻撃が空振りにはならない。
/// </summary>
class EnemyBoneAttackComponent {
public:
	// イベントのタグ名。.anim.json にこの名前で打つ
	static constexpr const char* kHitStartTag = "hit_start";
	static constexpr const char* kHitEndTag = "hit_end";

	explicit EnemyBoneAttackComponent(EnemyHitbox* hitbox);

	void BeginAttack(Enemy& enemy, const BoneAttackParams& params);
	void Update(Enemy& enemy, float deltaTime);

	/// <summary>
	/// 攻撃を中断して判定を消す。**必ずステートの Exit() から呼ぶこと**。
	/// 被弾などで攻撃モーションの途中でステートが切り替わると Update が回らなくなり、
	/// 判定が出しっぱなしのまま残ってしまう
	/// </summary>
	void Cancel(Enemy& enemy);

	bool IsFinished() const { return finished_; }
	/// <summary>判定が出る前の予備動作中か（チャージ演出の判定に使う）</summary>
	bool IsWindingUp() const;

private:
	void SetHitActive(Enemy& enemy, bool active);

	EnemyHitbox* hitbox_;
	BoneAttackParams params_;
	float timer_ = 0.0f;
	bool finished_ = true;
	bool hitActive_ = false;
	// このクリップがイベントを持っているか。持たない場合だけ比率で代用する
	bool useEvents_ = false;
};
