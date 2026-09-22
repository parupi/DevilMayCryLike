#pragma once
#include <Math/Vector3.h>
#include "GameObject/Effect/HitStop.h"
#include <vector>
#include <string>
#include <cstdint>

enum class AttackPosture {
	Stand, // 立ち状態
	Air, // 空中状態
};

/// <summary>
/// 攻撃中だけ体へ掛けるボーン補正1つぶん。
///
/// プレイヤーの斬りクリップは Alien.gltf の SwordSlash 1本しかないので、
/// 「技ごとにモーションを変える」のはここで差を付けて作る。
/// 腕を上げる・体をひねる・剣を持つ手を返す、といった補正を技ごとに持たせる。
/// 実際に回すのは Engine 側の BoneModifier（Skeleton::AddJointRotation）。
/// </summary>
struct AttackBonePose {
	/// ジョイント名。Alien.gltf なら "UpperArm.R" / "LowerArm.R" / "Palm.R" / "Torso" など
	/// （一覧は Animation ウィンドウの「ジョイント」で見られる）
	std::string boneName;
	/// 補正回転[度]
	Vector3 euler{};
	/// 0〜1。ボーンごとの効き具合
	float weight = 1.0f;
	/// 回す軸の空間。0=Local(ボーン自身の軸) / 1=Parent(親の軸) / 2=Model(モデル空間)
	/// Engine の BoneRotationSpace と同じ並び
	int32_t space = 0;
};

/// 1つの攻撃に持たせられるボーン補正の数。増やすなら攻撃エディタの上限も一緒に見ること
inline constexpr int32_t kMaxAttackBonePoses = 6;

enum class ReactionType {
	HitStun,   // のけぞり
	Knockback, // 吹っ飛び
	Launch     // 打ち上げ
};

/// <summary>
/// プレイヤーの攻撃の見た目の種類（剣の軌跡の色・溜めの光・技ごとの追加演出）。PlayerAttackEffect が使う。
/// 攻撃エディタの "VFX Style" の並びと同じ
/// </summary>
enum class AttackVfxStyle : int32_t {
	Auto,   // 攻撃の性能から決める
	Slash,  // 通常斬り（白＋青）
	Heavy,  // 強攻撃（白＋金）
	Thrust, // 突進（前方の風・衝撃リング・集中線）
	Launch, // 打ち上げ（長い軌跡・地面の衝撃・上昇する光）
	Slam,   // 叩きつけ（地面の衝撃波・ひび・土煙）
	Count,
};

/// <summary>
/// ノックバックの向きの決め方（仕様書 §4）。
/// 基本は「攻撃者 → 被弾者」で、攻撃ごとに向きを固定したいときだけ他を選ぶ。
/// </summary>
enum class KnockbackDirection : int32_t {
	AwayFromAttacker, // 攻撃者から離れる（既定）
	AttackerForward,  // 攻撃者の正面へ。横から当てても同じ向きへ飛ばしたいとき
	Upward,           // 真上へ。水平成分を捨てる
	TowardAttacker,   // 引き寄せ（§5.5）。コンボ維持・距離調整用
	Count
};

/// <summary>
/// ノックバック中に重ねて攻撃を受けたときの合成方法（仕様書 §8）。
/// </summary>
enum class KnockbackBlend : int32_t {
	Override, // 上書き。強い攻撃を受けたときの反応が分かりやすい
	Additive, // 加算 + 上限(maxSpeed)。多段ヒット攻撃と相性がよい
	Count
};

/// <summary>
/// 攻撃1回ぶんのノックバック性能（仕様書 §3）。
/// 攻撃を出す側（AttackData）と受ける側へ渡す情報（DamageInfo）で同じものを使う。
///
/// power / verticalPower は **速度[m/s]**。
/// エディタ上の調整値は従来どおり ImpulseForce / UpwardRatio（power に対する割合）で、
/// 読み込み時にここへ変換する。
/// </summary>
struct KnockbackData {
	ReactionType type = ReactionType::HitStun;

	float power = 0.0f;          // 水平方向の強さ[m/s]
	float verticalPower = 0.0f;  // 上方向の強さ[m/s]
	float duration = 0.0f;       // ノックバックの時間[秒]。0 なら種類ごとの既定値を使う
	float deceleration = 1.0f;   // 減速カーブの鋭さ。1.0 で仕様書どおりの直線 power*(1-t)
	float maxSpeed = 0.0f;       // 合成後の水平速度の上限[m/s]。0 なら無制限
	float torque = 0.0f;         // 吹っ飛び中の回転量[度/秒]
	float stunTime = 0.0f;       // のけぞり・操作不能の時間[秒]

	KnockbackDirection direction = KnockbackDirection::AwayFromAttacker;
	KnockbackBlend blend = KnockbackBlend::Override;

	/// <summary>開始時に元の速度を捨てるか。false なら残っている速度に足す</summary>
	bool overrideVelocity = true;
	/// <summary>この攻撃で相手を地面から浮かせてよいか。受ける側の耐性と AND される</summary>
	bool canLaunch = true;
};

/// <summary>
/// 敵ごとのノックバック耐性（仕様書 §9）。
/// 「すべての敵を同じように吹き飛ばさない」ための重み付け。
/// </summary>
struct KnockbackResistance {
	/// <summary>0.0 = 素通し、1.0 = 完全無効。受けた power / verticalPower に (1-resistance) が掛かる</summary>
	float resistance = 0.0f;
	/// <summary>のけぞる（被弾リアクションのステートへ入って行動が中断される）</summary>
	bool canStagger = true;
	/// <summary>吹き飛ぶ</summary>
	bool canBlowAway = true;
	/// <summary>打ち上がる</summary>
	bool canLaunch = true;
};

// 攻撃の情報
struct AttackData {
	std::string name = ""; // 名前
	// 挙動系
	int32_t pointCount = 0;                      // 制御点の数
	std::vector<Vector3> controlPoints;      // 制御点（Vector3）
	std::vector<Vector3> controlRotations;   // 制御点の回転（Vector3）

	// 移動系
	Vector3 moveVelocity{};                  // 攻撃中の移動速度

	// タイマー系
	float totalDuration = 0.0f;              // 攻撃全体にかかる時間
	float preDelay = 0.0f;                   // 予備動作の時間
	float attackDuration = 0.0f;             // 攻撃中の時間
	float postDelay = 0.0f;                  // 技後の硬直時間
	float nextAttackDelay = 0.0f;            // 次のアクションを受け付ける時間

	// その他
	bool drawDebugControlPoints = false;     // 制御点のデバッグ描画フラグ

	bool isMove = false;                     // 攻撃中に移動するかどうか(モーション用のフラグ)

	// 攻撃力
	float damage = 0.0f;                      // ダメージ値

	// 派生先
	AttackPosture posture = AttackPosture::Stand;

	// HitStop
	float hitStopTime = 0.0f;
	float hitStopIntensity = 0.0f;
	HitStopStrength hitStopStrength = HitStopStrength::Heavy; // 攻撃の強さ（止まり方の強弱）

	/// <summary>攻撃を受けた側に送るノックバック性能（仕様書 §3）</summary>
	KnockbackData knockback{};

	// ── 多段ヒット・当たり判定 ──
	// 攻撃判定が出ている間に何回当たり直すか（Attack Duration を等分する）。1 なら従来通り1回
	int32_t hitCount = 1;
	// 武器の当たり判定の大きさの倍率
	float hitboxScale = 1.0f;
	// 最終段だけ別の性能にするか（hitCount が2以上のときだけ効く）
	bool useFinalHit = false;
	float finalDamage = 0.0f;
	float finalHitStopTime = 0.0f;
	/// <summary>最終段のノックバック性能（useFinalHit が true のときだけ使う）</summary>
	KnockbackData finalKnockback{};

	// ── 溜め ──
	// ボタンを押し続けている間、構え（予備動作の終わり）で止めて溜める
	bool isCharge = false;
	float chargeMinTime = 0.2f;       // これより短く離すと溜め無し（倍率1.0）
	float chargeMaxTime = 1.0f;       // ここまで溜めると最大倍率
	float chargeDamageScale = 1.0f;   // 最大まで溜めたときのダメージ倍率
	float chargeImpulseScale = 1.0f;  // 同・吹き飛ばしの強さの倍率
	float chargeHitStopScale = 1.0f;  // 同・ヒットストップの長さの倍率

	// ── 無敵 ──
	// 攻撃の出始めから被弾しない時間[秒]
	float invincibleTime = 0.0f;

	// ── 見た目 ──
	// 剣の軌跡の色・溜めの光・技ごとの追加演出の種類（PlayerAttackEffect）。Auto は攻撃の性能から決める
	AttackVfxStyle vfxStyle = AttackVfxStyle::Auto;

	// ── アニメーション（攻撃ごとに体のモーションを変える）──
	// 既定値はどれも「今までと同じ動き」になる値にしてある
	/// 再生するクリップ名。空なら Player::kClipAttack（共通の斬り）
	std::string animClip;
	/// 振り切る瞬間のクリップ上の位置(0〜1)。0 なら Player::kAttackClipImpactRatio。
	/// クリップを差し替えたとき、体の振り抜きと剣の振り抜きを合わせるために使う
	float animImpactRatio = 0.0f;
	/// 攻撃の長さへ合わせた再生速度に、さらに掛ける倍率
	float animSpeedScale = 1.0f;
	/// このクリップへ入るときのブレンド時間[秒]
	float animBlendTime = 0.05f;

	/// 攻撃中だけ掛けるボーン補正
	std::vector<AttackBonePose> bonePoses;
	/// 補正の立ち上がり／抜け（構え＋振りを 1 とした割合）。
	/// 0 にすると出た瞬間にポーズが変わるので、少し取って滑らかに入れる
	float poseFadeIn = 0.25f;
	float poseFadeOut = 0.3f;
};

struct DamageInfo {
	float damage = 0.0f;

	Vector3 hitPosition{};
	Vector3 hitNormal{};
	Vector3 attackerPosition{};
	/// <summary>攻撃者の正面（水平・正規化済み）。KnockbackDirection::AttackerForward で使う</summary>
	Vector3 attackerForward{};
	/// <summary>解決済みのノックバック方向（正規化済み）。KnockbackSolver が埋める</summary>
	Vector3 direction{};

	/// <summary>ノックバック性能。耐性を適用したあとの値が入る</summary>
	KnockbackData knockback{};
};

struct MoveIntent {
	Vector3 moveDir{}; // 入力方向
	float moveScale = 0.0f; // 0.0f〜1.0f
	bool jump =	false; // このフレームでジャンプしたい
	bool dash = false; // ダッシュしたい
};
