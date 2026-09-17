#include "CombatHitResolver.h"
#include <algorithm>

namespace {
	// ReactionType ごとの補正（CombatHit::ApplyTypeScale）。
	// 攻撃データの ImpulseForce / UpwardRatio は種類をまたいで1つしかないため、
	// 「のけぞりなのに吹き飛びと同じだけ滑る」ことがないようにここで重み付けする。
	// 値は作り直す前の実装（EnemyStateKnockBack）が持っていた倍率をそのまま引き継いでいるので、
	// すでに調整済みの攻撃の手触りは変わらない
	constexpr float kPowerScale[3] = {
		0.25f, // HitStun : 少し後退するだけ
		1.0f,  // Knockback
		1.0f,  // Launch
	};
	constexpr float kVerticalScale[3] = {
		1.0f, // HitStun : 地上では浮かせない（KnockbackComponent が接地中は上方向を捨てる）。
		      //           空中の相手に当たったときだけ効いて、空中コンボで敵を留める
		1.0f, // Knockback
		1.4f, // Launch : 空中コンボへ繋ぐぶん高く上げる
	};

	int32_t TypeIndex(ReactionType type) {
		return std::clamp(static_cast<int32_t>(type), 0, 2);
	}

	Vector3 HorizontalUnit(const Vector3& v, const Vector3& fallback) {
		Vector3 horizontal{ v.x, 0.0f, v.z };
		const float length = Length(horizontal);
		if (length < 0.0001f) return fallback;
		return horizontal * (1.0f / length);
	}
}

namespace CombatHit {

Vector3 ResolveDirection(const KnockbackData& knockback, const Attacker& attacker, const Vector3& targetPosition) {
	// 向きが取れないときの保険。攻撃者の正面 → それも無ければ +Z
	const Vector3 fallback = HorizontalUnit(attacker.forward, Vector3{ 0.0f, 0.0f, 1.0f });

	switch (knockback.direction) {
	case KnockbackDirection::AttackerForward:
		return fallback;

	case KnockbackDirection::Upward:
		// 水平には飛ばさない。上方向は verticalPower が担当する
		return Vector3{ 0.0f, 0.0f, 0.0f };

	case KnockbackDirection::TowardAttacker:
		// 引き寄せ（仕様書 §5.5）
		return HorizontalUnit(attacker.position - targetPosition, fallback);

	case KnockbackDirection::AwayFromAttacker:
	default:
		// 基本形。**水平化してから正規化する**。
		// 3Dのまま正規化すると、段差や打ち上げ中の高低差のぶんだけ斜め上下へ飛んでしまう
		return HorizontalUnit(targetPosition - attacker.position, fallback);
	}
}

KnockbackData ApplyTypeScale(KnockbackData knockback) {
	const int32_t index = TypeIndex(knockback.type);
	knockback.power *= kPowerScale[index];
	knockback.verticalPower *= kVerticalScale[index];
	return knockback;
}

KnockbackData ApplyResistance(KnockbackData knockback, const KnockbackResistance& resistance) {
	// ── ① リアクションの種類を許可されたところまで落とす ──
	// 打ち上げ → 吹き飛び → のけぞり の順に軽くする
	if (knockback.type == ReactionType::Launch && !(resistance.canLaunch && knockback.canLaunch)) {
		knockback.type = ReactionType::Knockback;
	}
	if (knockback.type == ReactionType::Knockback && !resistance.canBlowAway) {
		knockback.type = ReactionType::HitStun;
	}

	// ── ② 強さを削る ──
	const float scale = std::clamp(1.0f - resistance.resistance, 0.0f, 1.0f);
	knockback.power *= scale;
	knockback.verticalPower *= scale;
	knockback.torque *= scale;
	knockback.stunTime *= scale;
	if (knockback.maxSpeed > 0.0f) {
		knockback.maxSpeed *= scale;
	}
	return knockback;
}

Result Resolve(const AttackData& attack, const Attacker& attacker,
	const Vector3& targetPosition, const KnockbackResistance& resistance) {

	DamageInfo raw;
	raw.damage = attack.damage;
	raw.knockback = attack.knockback;
	raw.attackerPosition = attacker.position;
	raw.attackerForward = attacker.forward;
	raw.hitPosition = targetPosition;

	return Resolve(raw, attacker, targetPosition, resistance);
}

Result Resolve(const DamageInfo& rawInfo, const Attacker& attacker,
	const Vector3& targetPosition, const KnockbackResistance& resistance) {

	Result result;
	result.info = rawInfo;
	result.info.attackerPosition = attacker.position;
	result.info.attackerForward = attacker.forward;

	// ③ 方向（仕様書 §20）
	result.info.direction = ResolveDirection(rawInfo.knockback, attacker, targetPosition);

	// ④ 種類ごとの補正 → 耐性
	//    種類を落としてから補正を掛けたいので、耐性のほうが先
	KnockbackData knockback = ApplyResistance(rawInfo.knockback, resistance);
	knockback = ApplyTypeScale(knockback);
	result.info.knockback = knockback;

	// のけぞりすら許さない相手（ボスなど）は行動を中断しない。
	// 速度だけは渡すので、ダメージが通っていることは体が押されることで伝わる
	result.causesReaction = resistance.canStagger;

	return result;
}

} // namespace CombatHit
