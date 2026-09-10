#pragma once
#include <string>
#include <Math/Vector3.h>
#include "GameObject/Character/CharacterStructs.h"
#include "GameObject/Effect/AttackTelegraph.h"

class Enemy;
class EnemyHitbox;

/// <summary>
/// 「体そのもので殴る」攻撃のパラメータ。武器を振る攻撃（MeleeAttackParams）の置き換え。
/// 判定の位置はアニメーションのポーズが決めるので、制御点は持たない。
/// </summary>
struct BoneAttackParams {
	std::string jointName = "Head";     // 判定を付けるジョイント

	// 判定をジョイントではなく **体の正面基準** で出す（jointName は無視される）。
	// ブレスのように「口から前方へまっすぐ伸びる」判定は、骨のローカル軸に従わせると
	// あらぬ方向へ伸びてしまうので、こちらを使う。
	// このとき offset / halfExtents は +Z がプレイヤー側・+Y が上の座標系で解釈される
	bool orientToBody = false;

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

	// 予備動作（判定が出るまでの溜め）を何秒ぶん引き伸ばすか。
	// クリップの溜め部分だけをゆっくり流し、振り抜きは等速のまま残すので、
	// 「長く構えてから鋭く振る」＝見てから回避できる大振りになる。
	// 攻撃全体の長さは duration + extraWindupTime になる。
	//
	// 溜めの境目は **hitStartRatio** で決まる。イベント駆動のクリップでも
	// ここを伸ばすなら、hitStartRatio を hit_start の位置に合わせておくこと
	// （ずれるとクリップが振り抜いた後も溜め扱いのまま止まって見える）
	float extraWindupTime = 0.0f;

	// 地面に出す予兆（赤いマーカー）。shape が None なら出さない。
	// 予備動作の間ずっと出したままにして、判定が出る瞬間に塗りが外枠へ届く。
	// 大きさは halfExtents と同じくスケール1基準で書く（BeginAttack が配置スケールを掛ける）
	AttackTelegraphParams telegraph;
};

/// <summary>
/// ボーン追従のヒットボックスで殴る攻撃を実行するコンポーネント。
///
/// クリップは等速で流し、判定のON/OFFは **アニメーションイベント** で取る
/// （Animation ウィンドウで hit_start / hit_end を打って .anim.json に保存する）。
/// イベントが無いクリップでは duration に対する比率で代用するので、
/// イベントを打つ前でも攻撃が空振りにはならない。
///
/// extraWindupTime を入れると攻撃は「溜め → 本編」の2段になる。
/// 溜めの間はクリップを遅く流して判定も突進も出さないので、
/// プレイヤーは大振りを見てから回避できる。
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
	/// <summary>判定が出ている最中か（ブレスの炎を吹き続ける等、本編の演出に使う）</summary>
	bool IsHitActive() const { return hitActive_; }
	/// <summary>判定が出る前の予備動作中か（チャージ演出の判定に使う）</summary>
	bool IsWindingUp() const;
	/// <summary>
	/// 予備動作の進み具合（0→1、振り抜く直前で1）。
	/// 溜めが進むほど演出を強める（発光・チャージ粒子）のに使う。
	/// 予備動作中以外の値には意味がないので IsWindingUp() と併せて見ること
	/// </summary>
	float GetWindupProgress() const;
	/// <summary>
	/// 予兆マーカーの進み具合（0→1、判定が出る瞬間に1）。
	/// 溜めを伸ばした攻撃はその溜め全体、伸ばしていない攻撃はクリップ側の溜めが尺になる。
	/// **イベント駆動のクリップでは hitStartRatio が hit_start の位置と合っている前提**
	/// （ずれると塗りが届く前／届いた後に判定が出る）
	/// </summary>
	float GetTelegraphProgress() const;

private:
	void SetHitActive(Enemy& enemy, bool active);
	// 本編（判定が出てから終わるまで）のクリップ再生速度を決める
	void ApplyStrikeAnimSpeed(Enemy& enemy);
	// 予兆マーカーを敵の足元へ出し直す。判定が出たら呼ぶのをやめて自動で畳ませる
	void UpdateTelegraph(Enemy& enemy);

	EnemyHitbox* hitbox_;
	BoneAttackParams params_;
	float timer_ = 0.0f;
	bool finished_ = true;
	bool hitActive_ = false;
	// このクリップがイベントを持っているか。持たない場合だけ比率で代用する
	bool useEvents_ = false;

	// ── 予備動作（溜め）──
	bool  inWindup_ = false;
	float windupTimer_ = 0.0f;       // 溜めの経過[s]
	float windupDuration_ = 0.0f;    // 溜めの実時間[s]（クリップの溜め + extraWindupTime）
	float windupClipSeconds_ = 0.0f; // クリップ側の溜めの長さ[s]。本編はここから始まる

	// 判定を出したいかどうか。AnimationPlayer は発火したイベントを毎フレーム捨てるので、
	// 溜め中に飛んできた hit_start もここへ受けて本編まで持ち越す
	bool hitRequested_ = false;
};
