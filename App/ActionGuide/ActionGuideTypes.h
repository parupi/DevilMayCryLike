#pragma once
#include <cstdint>
#include <string>
#include <vector>

/// <summary>
/// アクションガイド（画面右下の操作案内）で共有する型。
/// 仕様書: ActionGuideSystem_Spec.md
///
/// 依存の向きは「戦闘ロジック → (読むだけ) → ガイド」。戦闘側はこのフォルダを一切知らない
/// </summary>

/// <summary>その行動を今どう扱えるか</summary>
enum class ActionAvailability : uint8_t {
	Unavailable, ///< 出せない（表示しない）
	Preview,     ///< この先出せるが、今押しても出ない（薄く出す）
	Buffered,    ///< 今押すと先行入力として覚えられ、受付に入った瞬間に出る
	Executable,  ///< 今押せばすぐ出る
};

/// <summary>論理入力。物理ボタンへの対応は InputIconProvider が決める</summary>
enum class InputAction : uint8_t {
	None,
	Attack,       ///< 通常攻撃（パッド Y / キーボード J）
	StrongAttack, ///< 強攻撃（パッド X / キーボード K）
	Jump,         ///< パッド A / キーボード SPACE
	Dodge,        ///< パッド RT / キーボード 左SHIFT
	LockOn,       ///< パッド RB / キーボード P
};

/// <summary>押し方</summary>
enum class InputGesture : uint8_t {
	Press,     ///< 押す
	Hold,      ///< 長押し（溜め攻撃・ロックオン）
	Release,   ///< 離す（溜め中）
	WaitPress, ///< 少し待ってから押す（同じボタンで時間により変わる派生）
};

/// <summary>一緒に入れるスティックの向き</summary>
enum class StickHint : uint8_t {
	None,
	Forward, ///< 前（敵の方）
	Back,    ///< 後ろ
};

/// <summary>
/// 1項目の入力の組み合わせ（仕様書の InputBindingExpression の簡易版）。
/// 「ロックオンを押しながら + スティック前 + 攻撃」のような組み合わせを表す
/// </summary>
struct InputBinding {
	InputAction action = InputAction::None;
	InputGesture gesture = InputGesture::Press;
	StickHint stick = StickHint::None;
	bool withLockOn = false;

	bool operator==(const InputBinding&) const = default;
};

using ActionGuideID = uint32_t;

/// <summary>
/// 候補の種類。優先度の既定値（エディタで調整できる）と、どの判定で可否を決めるかに使う
/// </summary>
enum class ActionGuideCategory : uint8_t {
	ChargeRelease, ///< 溜め中に離す
	Counter,       ///< ジャスト回避直後のカウンター
	ComboBranch,   ///< 今の攻撃からの派生（今押せば出るもの）
	BranchLater,   ///< 同じボタンで、少し待つと出る派生
	RootInterrupt, ///< 攻撃中に別のボタンで割り込むルート攻撃
	RootAttack,    ///< 攻撃していないときのルート攻撃
	Dodge,         ///< 回避（攻撃中のキャンセルを含む）
	Jump,          ///< ジャンプ
	LockOn,        ///< ロックオン

	Count,
};
constexpr int32_t kActionGuideCategoryCount = static_cast<int32_t>(ActionGuideCategory::Count);

/// <summary>表示する1行</summary>
struct ActionGuideEntry {
	ActionGuideID id = 0;
	InputBinding binding;
	std::string displayName;
	ActionAvailability availability = ActionAvailability::Unavailable;
	int32_t priority = 0;
	/// <summary>先行入力としてもう押してある（Buffered のときだけ意味がある）</summary>
	bool queued = false;

	bool operator==(const ActionGuideEntry&) const = default;
};

/// <summary>ガイドが描くもの一式。UI はこれだけを見る</summary>
struct ActionGuideViewModel {
	std::vector<ActionGuideEntry> entries;
	bool visible = false;

	bool operator==(const ActionGuideViewModel&) const = default;
};
