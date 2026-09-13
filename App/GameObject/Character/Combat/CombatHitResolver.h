#pragma once
#include "Math/Vector3.h"
#include "GameObject/Character/CharacterStructs.h"

/// <summary>
/// 攻撃が当たったときの「ダメージ → 方向 → 耐性 → ノックバック性能」を1箇所にまとめたもの
/// （仕様書 §20 の ①〜④、§21 の HitSystem / KnockbackSystem に当たる部分）。
///
/// これが無かった頃は、敵ごとの OnCollisionEnter と Player::TakeDamage が
/// それぞれ別のやり方で方向を求めていた（敵は3Dのまま・プレイヤーは水平化）ため、
/// 高低差があると敵だけ斜め上へ飛ぶ、といった食い違いが起きていた。
///
/// 実際に速度を与えるのは KnockbackComponent、演出は HitEffectSystem の担当。
/// ここは「どんなノックバックになるか」を決めるだけで、状態は持たない。
/// </summary>
namespace CombatHit {

	/// <summary>攻撃を出した側の情報</summary>
	struct Attacker {
		Vector3 position{};          // ワールド座標
		Vector3 forward{};           // 水平な正面（KnockbackDirection::AttackerForward 用）
	};

	/// <summary>ヒットの解決結果</summary>
	struct Result {
		/// <summary>受け取る側へ渡す情報。direction と knockback は解決済み</summary>
		DamageInfo info{};
		/// <summary>
		/// 被弾リアクション（のけぞり・吹き飛びのステート）へ入ってよいか。
		/// false の敵（ボスなど）は行動を中断せず、ノックバックの速度だけを受ける。
		/// </summary>
		bool causesReaction = true;
	};

	/// <summary>
	/// ノックバックの向きを決める（仕様書 §4）。返り値は水平・正規化済み。
	/// 向きが決まらないとき（真上から重なっているなど）は攻撃者の正面、それも無ければ +Z。
	/// </summary>
	Vector3 ResolveDirection(const KnockbackData& knockback, const Attacker& attacker, const Vector3& targetPosition);

	/// <summary>
	/// ReactionType ごとの補正を掛ける。
	/// のけぞりは水平に押す量を抑え、打ち上げは上方向を強めに出す。
	/// 攻撃データ側は種類を切り替えても ImpulseForce / UpwardRatio を付け直さなくてよくなる。
	/// </summary>
	KnockbackData ApplyTypeScale(KnockbackData knockback);

	/// <summary>
	/// 敵の耐性を適用する（仕様書 §9）。
	/// 許可されていないリアクションは1段階ずつ軽いものへ落とし、強さに (1 - resistance) を掛ける。
	/// </summary>
	KnockbackData ApplyResistance(KnockbackData knockback, const KnockbackResistance& resistance);

	/// <summary>
	/// プレイヤーの攻撃データから、敵へ渡すヒット情報を作る（仕様書 §20 ①〜④）。
	/// </summary>
	Result Resolve(const AttackData& attack, const Attacker& attacker,
		const Vector3& targetPosition, const KnockbackResistance& resistance);

	/// <summary>
	/// 敵の攻撃のように、すでに KnockbackData が埋まっている情報を解決する。
	/// 方向・種類補正・耐性だけを掛け直す。
	/// </summary>
	Result Resolve(const DamageInfo& rawInfo, const Attacker& attacker,
		const Vector3& targetPosition, const KnockbackResistance& resistance);

} // namespace CombatHit
