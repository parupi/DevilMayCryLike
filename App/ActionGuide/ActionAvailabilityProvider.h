#pragma once
#include "ActionGuideTypes.h"
#include "ActionGuideContext.h"

#include <string>
#include <vector>

class Player;
class ActionGuideCatalog;
struct ActionGuideSettings;

/// <summary>評価の対象になる1項目。候補を集める段階で、入力の組み合わせと種類まで決まる</summary>
struct ActionGuideCandidate {
	ActionGuideID id = 0;
	InputBinding binding;
	ActionGuideCategory category = ActionGuideCategory::RootAttack;
};

/// <summary>
/// 既存の遷移条件を参照するアダプター（仕様書 §5.3）。
///
/// 仕様書の Evaluate(id, context) との違い: このゲームでは同じ攻撃が
/// 「コンボの派生」としても「別ボタンの割り込み」としても出るので、id だけでは
/// どの判定を見ればよいか決まらない。そこで候補（id ＋ 種類 ＋ 入力）を渡す形にしている。
/// どちらの関数も状態を変えない
/// </summary>
class IActionAvailabilityProvider
{
public:
	virtual ~IActionAvailabilityProvider() = default;

	/// <summary>今の状態で候補になる行動を集める（仕様書 §6-2）</summary>
	virtual void CollectCandidates(const ActionGuideContext& context, std::vector<ActionGuideCandidate>& out) const = 0;

	/// <summary>候補を評価する（仕様書 §6-3）。queued には「もう先行入力で押してあるか」を返す</summary>
	virtual ActionAvailability Evaluate(const ActionGuideCandidate& candidate, const ActionGuideContext& context, bool& queued) const = 0;
};

/// <summary>
/// プレイヤーの戦闘ロジック（PlayerCombat / PlayerStateAttack / ステート）を読むだけの実装。
/// 判定はすべて既存の関数を呼ぶ。条件式をここに書き写さないこと
/// </summary>
class PlayerActionAvailability : public IActionAvailabilityProvider
{
public:
	PlayerActionAvailability(Player& player, ActionGuideCatalog& catalog, const ActionGuideSettings& settings)
		: player_(player), catalog_(catalog), settings_(settings) {}

	void CollectCandidates(const ActionGuideContext& context, std::vector<ActionGuideCandidate>& out) const override;
	ActionAvailability Evaluate(const ActionGuideCandidate& candidate, const ActionGuideContext& context, bool& queued) const override;

private:
	/// <summary>このボタンで今出るルート攻撃を、スティックの向きの組み合わせごとに集める</summary>
	void CollectRootAttacks(InputAction button, ActionGuideCategory category, std::vector<ActionGuideCandidate>& out) const;
	/// <summary>攻撃名から候補を作る（ガイドに出さない攻撃なら false）</summary>
	bool MakeAttackCandidate(const std::string& attackName, InputAction button, ActionGuideCategory category, ActionGuideCandidate& out) const;

	Player& player_;
	ActionGuideCatalog& catalog_;
	const ActionGuideSettings& settings_;
};
