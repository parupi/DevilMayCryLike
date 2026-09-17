#pragma once
#include "Math/Vector3.h"
#include "GameObject/Character/CharacterStructs.h"

/// <summary>
/// ノックバックの速度・減衰・時間だけを持つ部品（仕様書 §6, §7, §8）。
///
/// 仕様書 §6 の「通常移動とノックバックを速度として分離する」を実現するためのクラス。
/// 所有者（Enemy / Player）は自分の移動速度とは **別に** これを持ち、
/// 最終的な移動を moveVelocity + knockbackVelocity で求める。
///
/// これを分けたことで
/// - ボスのように被弾リアクションのステートへ入らない敵でも位置だけは押される
/// - 敵と味方で減速の式が食い違わない
/// ようになっている（以前は Enemy / Player それぞれのステートが自前で速度を持っていた）。
///
/// 垂直方向は重力で落とすだけなので、接地しているかは所有者が SetGrounded() で教える。
/// </summary>
class KnockbackComponent {
public:
	/// <summary>
	/// KnockbackData::duration が 0 のときに使う、種類ごとの既定のノックバック時間[秒]。
	/// HitStun / Knockback / Launch の順。
	/// </summary>
	static float DefaultDuration(ReactionType type);

	// ======================
	// 開始・追撃
	// ======================

	/// <summary>
	/// ノックバックを開始する。
	/// direction は水平方向のみ使い、上方向は KnockbackData::verticalPower が担当する。
	/// </summary>
	/// <param name="inheritVelocity">
	/// overrideVelocity が false のときに引き継ぐ元の速度。捨てる場合は無視される
	/// </param>
	void Begin(const KnockbackData& data, const Vector3& direction, const Vector3& inheritVelocity = {});

	/// <summary>
	/// ノックバック中にもう一度攻撃を受けたときの合成（仕様書 §8）。
	/// KnockbackBlend::Override なら Begin と同じ上書き、Additive なら加算して maxSpeed で頭打ちにする。
	/// </summary>
	void AddHit(const KnockbackData& data, const Vector3& direction);

	// ======================
	// 更新
	// ======================

	/// <summary>
	/// 速度を1フレーム進める。gravity は負の値（例: -9.8f）を渡す。
	/// 水平は時間ベースで減衰し（仕様書 §7）、垂直は重力で落ちる。
	/// </summary>
	void Update(float deltaTime, float gravity);

	/// <summary>接地状態を教える。接地中は下向きの速度を残さない（地面にめり込み続けるのを防ぐ）</summary>
	void SetGrounded(bool grounded) { grounded_ = grounded; }

	/// <summary>ノックバックを打ち切って速度を捨てる</summary>
	void Stop();

	/// <summary>
	/// 着地したときに呼ぶ。垂直を消し、水平を keepRatio 倍に落とす（滑って止まる見た目になる）。
	/// </summary>
	void OnLand(float keepRatio = 0.3f);

	/// <summary>
	/// 壁・移動範囲の境界に当たったときに呼ぶ（仕様書 §10）。
	/// inwardNormal（内側を向く単位ベクトル）の外向き成分だけを取り除く。
	/// 壁に押し付けられ続けるのを防ぐ。
	/// </summary>
	/// <returns>実際に速度を削ったか（＝壁にぶつかっていたか）。壁ヒット演出の判定に使う</returns>
	bool CancelOutward(const Vector3& inwardNormal);

	// ======================
	// 取得
	// ======================

	bool IsActive() const { return active_; }

	/// <summary>ノックバックで宙に浮いている最中か（のけぞりは地上扱いなので false）</summary>
	bool IsAirborne() const {
		return active_ && data_.type != ReactionType::HitStun && !grounded_;
	}

	/// <summary>
	/// 水平の減衰が終わっているか（仕様書 §7 の終了条件）。
	/// 速度を出し切って停止したあとも true のままにする（被弾リアクションの復帰判定に使うため）
	/// </summary>
	bool IsHorizontalFinished() const { return !active_ || elapsed_ >= Duration(); }

	/// <summary>
	/// のけぞりで動けない時間が残っているか（仕様書 §11）。
	/// ノックバックの速度が止まったあとも残り、追撃を受けると延長される。
	/// 速度の減衰時間（duration）とは別枠なので、短く飛ばして長く固める攻撃も作れる
	/// </summary>
	bool IsStunned() const { return stunRemain_ > 0.0f; }
	float GetStunRemain() const { return stunRemain_; }

	const Vector3& GetVelocity() const { return velocity_; }
	const KnockbackData& GetData() const { return data_; }
	ReactionType GetType() const { return data_.type; }
	float GetStunTime() const { return data_.stunTime; }
	float GetTorque() const { return data_.torque; }
	float GetElapsed() const { return elapsed_; }
	float Duration() const;

private:
	/// <summary>水平方向の減衰率[0,1]（仕様書 §7）。deceleration が 1.0 のとき 1-t の直線</summary>
	float Falloff() const;
	/// <summary>実際に与える上方向の強さ。のけぞりは接地中だけ 0 にする</summary>
	float VerticalPower() const;
	/// <summary>初速を maxSpeed で頭打ちにする</summary>
	void ClampSpeed();
	/// <summary>y を落として正規化する。長さが取れないときは false</summary>
	static bool HorizontalUnit(const Vector3& v, Vector3& out);

	KnockbackData data_{};

	Vector3 horizontalDir_{};      // 水平方向の向き（正規化済み）
	float horizontalSpeed_ = 0.0f; // 水平方向の初速[m/s]。減衰前の値
	Vector3 velocity_{};           // 現在のノックバック速度（水平の減衰後 + 垂直）

	float elapsed_ = 0.0f;
	// のけぞりの残り時間[秒]。速度が止まっても減り続けるので active_ とは独立に持つ
	float stunRemain_ = 0.0f;
	bool active_ = false;
	bool grounded_ = true;
};
