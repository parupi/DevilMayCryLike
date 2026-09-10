#pragma once
#include <string>
#include "Math/Vector3.h"
#include "GameObject/Character/CharacterStructs.h"

/// <summary>
/// 回避・ダッシュの調整値。
///
/// 「回避 → そのままダッシュ」を1つの連続した移動アクションとして扱うため、
/// 回避側とダッシュ側のパラメータを1か所にまとめている。
/// 実体は GlobalVariables（Resource/GlobalVariables/Player/PlayerDodge.json）にあり、
/// ImGui の GlobalVariables ウィンドウからそのまま調整できる。
/// </summary>
struct PlayerDodgeParams {
	/// GlobalVariables のグループ名（＝保存されるファイル名）
	static constexpr const char* kGroupName = "PlayerDodge";
	/// 保存先ディレクトリ（Resource/GlobalVariables/<ここ>/）
	static constexpr const char* kDirectoryName = "Player";

	// ── 回避 ──
	float dodgeDuration = 0.25f;      // 回避からダッシュへ移行するまでの時間[秒]
	float dodgeSpeed = 19.0f;         // 回避中の移動速度（通常移動は10）
	float invincibleTime = 0.22f;     // 回避開始からの無敵時間[秒]
	float cooldown = 0.15f;           // 次に回避できるようになるまでの時間[秒]

	// ── ダッシュ ──
	float dashSpeed = 26.0f;          // ダッシュ中の移動速度
	float dashDuration = 1.2f;        // ダッシュを続けられる上限[秒]
	float dashTurnRate = 5.0f;        // 進行方向をスティックへ寄せる角速度[rad/秒]
	float dashInputGrace = 0.15f;     // 入力を離してからダッシュが切れるまでの猶予[秒]

	// ── ジャスト回避 ──
	float justDodgeSlowTime = 0.08f;  // スローモーションの長さ[秒]（実時間）
	float justDodgeTimeScale = 0.25f; // スローモーション中の時間倍率
	float justDodgeShake = 0.22f;     // ジャスト回避時にカメラへ加えるトラウマ量
	float justDodgeInvincibleAdd = 0.15f; // ジャスト回避成功時に伸ばす無敵時間[秒]

	// ── 演出 ──
	float dashFovPunch = 0.05f;       // ダッシュ開始時に一時的に足す画角[rad]（約3度）
	float trailLifetime = 0.22f;      // 残像（リボン）が消えるまでの時間[秒]

	/// GlobalVariables へ項目を登録し、保存済みの値があれば読み込む（Player::Initialize から1回）
	void RegisterAndLoad();
	/// GlobalVariables の現在値をメンバへ取り込む（毎フレーム先頭で呼ぶ）
	void Apply();
};

/// <summary>
/// 回避・ダッシュの実行時状態。
///
/// Dodge / JustDodge / Dash の3ステートがまたいで参照するので Player が持つ。
/// ステートに持たせると、ジャスト回避で Dodge を抜けて戻ってきたときに
/// 回避方向と経過時間が消えてしまう。
/// </summary>
struct PlayerDodgeRuntime {
	/// 回避・ダッシュの進行方向（水平・単位ベクトル）
	Vector3 direction{ 0.0f, 0.0f, 1.0f };
	/// 回避開始からの経過時間[秒]。ジャスト回避を挟んでも積算し続ける
	float elapsed = 0.0f;
	/// ジャスト回避から Dodge へ戻る途中か。true の間は Enter で回避をやり直さない
	bool resuming = false;
	/// この回避で既にジャスト回避したか（1回の回避につき1度だけ）
	bool justDodgeUsed = false;
	/// 被弾側（Player::TakeDamage）が立てる「次の更新でジャスト回避へ入れ」の印
	bool justDodgePending = false;
	/// ジャスト回避のきっかけになった攻撃。演出の向きを決めるのに使う
	DamageInfo justDodgeInfo{};
};
