#pragma once
#include <Math/Vector3.h>
#include "GameObject/Effect/AttackTelegraph.h"

class Enemy;

/// <summary>
/// 攻撃の狙い（体の向きと突進の進路）を、予兆を出した場所に固定する。
///
/// 以前は予兆が消えた後も敵がプレイヤーへ向き直り、突進もプレイヤーを追っていたので、
/// 予兆を見て範囲の外へ逃げても、攻撃の方が曲がってきて当たっていた。
/// 振り始めの瞬間（予兆の塗りが外枠へ届いた瞬間）に
///   - 体の向きを予兆の向きへ揃えて止める（攻撃が終わるまで振り向かない）
///   - 突進は予兆の正面へまっすぐ進める。帯の予兆なら、その奥の端で止まる
/// ので、攻撃は予兆をなぞるように動く。
///
/// **当たり判定そのものは今までどおり武器・ボーンのコライダーが持つ。**
/// 予兆の図形をそのまま判定にすると、突進の体に触れていないのに被弾することになる。
/// 予兆はあくまで「この場所へ攻撃が来る」を先に見せるためのもの。
///
/// 攻撃コンポーネントからの使い方:
///   1. 攻撃の開始で Begin()
///   2. 予備動作の間、毎フレーム Aim()（予兆を出して、出した場所を控える）
///   3. 振り始め（＝判定が出る瞬間）に Lock()
///   4. 攻撃の終わり・中断で Finish()
/// </summary>
class EnemyAttackAim {
public:
	/// <summary>
	/// 攻撃の開始時に呼ぶ。params は配置スケールを掛けた後のもの。
	/// rush が true なら、帯（Rect）の予兆を「体が走り抜ける道」として扱う（Rush を参照）。
	/// 前の攻撃が畳まれずに残っていたら、ここで後始末してから始める
	/// </summary>
	void Begin(Enemy& enemy, const AttackTelegraphParams& params, bool rush);

	/// <summary>予備動作の間、毎フレーム呼ぶ。敵の足元へ予兆を出し、出した場所と向きを控える</summary>
	void Aim(Enemy& enemy, float progress);

	/// <summary>
	/// 振り始めに呼ぶ（2回目以降は何もしない）。最後に出した予兆の場所と向きで狙いを固定し、
	/// 体の向きもそこへ揃えて止める。予兆を出していなければ今の足元と向きで固定する
	/// </summary>
	void Lock(Enemy& enemy);
	bool IsLocked() const { return phase_ == Phase::Locked; }

	/// <summary>
	/// 固定した正面へ体を進める（プレイヤーは追わない）。
	/// 帯の予兆では、体の前端が帯の奥の端へ届いたところで止まり、
	/// rushTime のうちに奥まで届く速さは必ず出す（予兆より手前で止まったり、追い越したりしないように）
	/// </summary>
	void Rush(Enemy& enemy, float speed, float rushTime, float deltaTime);

	/// <summary>
	/// 攻撃の終わり・中断で呼ぶ。振る前なら予兆を閃光なしで消し、振った後なら体の向きの固定を解く。
	/// 振った後の予兆は閃光を出して畳まれている最中なので触らない
	/// </summary>
	void Finish(Enemy& enemy);

private:
	enum class Phase {
		Idle,   // 攻撃していない
		Aiming, // 予備動作中。予兆を出しながら敵と一緒に回る
		Locked, // 振り始めた後。場所と向きは固定
	};

	void SetAim(const Vector3& origin, const Vector3& forward);
	// 帯の予兆を「体が走り抜ける道」として扱うか
	bool SweepsRect() const { return rush_ && params_.shape == TelegraphShape::Rect; }
	// 固定した基準点から、体が正面へ進んだ距離[m]
	float GetTravel(Enemy& enemy) const;
	// 体の厚みの半分[m]。帯の手前の端（forwardOffset）を体の後ろ端とみなして求める
	float GetRushHalfDepth() const;
	// 体の前端が帯の奥の端へ届くまでに進む距離[m]
	float GetRushMaxTravel() const;

	AttackTelegraphParams params_;
	bool rush_ = false;
	Phase phase_ = Phase::Idle;

	Vector3 origin_{};                    // 予兆の基準点（敵の足元。Y は地面の高さ）
	Vector3 forward_{ 0.0f, 0.0f, 1.0f }; // 予兆の正面（水平・単位ベクトル）
};
