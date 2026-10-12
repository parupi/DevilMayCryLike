#include "ActionAvailabilityProvider.h"
#include "ActionGuideCatalog.h"
#include "ActionGuideSettings.h"

#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/StateMachine/PlayerStateMachine.h"

#include <Debugger/GlobalVariables.h>
#include <Math/Vector2.h>

#include <algorithm>

namespace {
	// 論理入力 ↔ 攻撃のボタン。Y が通常攻撃、X が強攻撃（PlayerInput と同じ割り当て）
	InputButton ButtonOf(InputAction action) {
		switch (action) {
		case InputAction::Attack:       return InputButton::Y;
		case InputAction::StrongAttack: return InputButton::X;
		default:                        return InputButton::None;
		}
	}
	InputAction ActionOf(InputButton button) {
		return (button == InputButton::X) ? InputAction::StrongAttack : InputAction::Attack;
	}

	StickHint StickOf(StickDirection direction) {
		switch (direction) {
		case StickDirection::ToEnemy:       return StickHint::Forward;
		case StickDirection::AwayFromEnemy: return StickHint::Back;
		default:                            return StickHint::None;
		}
	}

	bool IsChargeAttack(const std::string& attackName) {
		GlobalVariables& gv = GlobalVariables::GetInstance();
		return gv.HasItem(attackName, "IsCharge") && gv.GetValueRef<bool>(attackName, "IsCharge");
	}

	// 候補の重複（同じ攻撃・同じ種類）を避ける
	bool Contains(const std::vector<ActionGuideCandidate>& list, ActionGuideID id, ActionGuideCategory category) {
		return std::any_of(list.begin(), list.end(), [&](const ActionGuideCandidate& c) {
			return c.id == id && c.category == category;
		});
	}

	constexpr InputAction kAttackButtons[] = { InputAction::Attack, InputAction::StrongAttack };
}

bool PlayerActionAvailability::MakeAttackCandidate(const std::string& attackName, InputAction button,
	ActionGuideCategory category, ActionGuideCandidate& out) const
{
	const ActionGuideID id = catalog_.GetAttackId(attackName);
	if (!catalog_.IsVisibleInGuide(id)) return false;

	const PlayerCombat* combat = player_.GetCombat();
	const AttackNode& node = combat->GetAttackNode(attackName);

	out.id = id;
	out.category = category;
	out.binding.action = button;
	out.binding.stick = StickOf(node.condition.stick);
	out.binding.withLockOn = node.condition.requireLockOn;
	// 溜め攻撃は長押しして離すと出る
	out.binding.gesture = IsChargeAttack(attackName) ? InputGesture::Hold : InputGesture::Press;
	return true;
}

void PlayerActionAvailability::CollectRootAttacks(InputAction button, ActionGuideCategory category,
	std::vector<ActionGuideCandidate>& out) const
{
	const PlayerCombat* combat = player_.GetCombat();
	const InputButton inputButton = ButtonOf(button);
	// 接地は「このフレームの入力を処理したときの値」を使う（GetOnGround は更新後だと常に false）
	const bool onGround = player_.IsGroundedForActions();

	// スティックの向きで変わる技（ロックオン中の 前+攻撃 など）もあるので、
	// ニュートラル・前・後ろの3通りで実際の選び方（FindRootAttack）に問い合わせる
	const Vector2 sticks[] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, -1.0f } };
	for (const Vector2& stick : sticks) {
		const AttackNode* node = combat->FindRootAttack(inputButton, stick, onGround);
		if (!node) continue;

		const ActionGuideCategory resolved = node->condition.requireCounter ? ActionGuideCategory::Counter : category;
		ActionGuideCandidate candidate;
		if (!MakeAttackCandidate(node->name, button, resolved, candidate)) continue;
		if (Contains(out, candidate.id, candidate.category)) continue;
		out.push_back(candidate);
	}
}

void PlayerActionAvailability::CollectCandidates(const ActionGuideContext& context, std::vector<ActionGuideCandidate>& out) const
{
	PlayerCombat* combat = player_.GetCombat();
	if (!combat) return;

	const PlayerStateAttack* top = combat->GetCurrentAttackState();

	if (context.isAttacking && top) {
		const AttackNode& topNode = combat->GetAttackNode(top->GetAttackName());

		// 溜め中は「離す」と回避だけ。他のボタンは溜めの最中には効かない
		if (context.isCharging) {
			ActionGuideCandidate release;
			if (MakeAttackCandidate(top->GetAttackName(), ActionOf(topNode.condition.button),
				ActionGuideCategory::ChargeRelease, release)) {
				release.binding.gesture = InputGesture::Release;
				release.binding.stick = StickHint::None;
				release.binding.withLockOn = false;
				out.push_back(release);
			}
		} else {
			for (InputAction button : kAttackButtons) {
				const InputButton inputButton = ButtonOf(button);

				// 今の攻撃の派生（実際に出すときと同じ選び方で、今押したら出るもの）
				const std::string branch = top->PeekBranch(*combat, inputButton);
				if (!branch.empty()) {
					ActionGuideCandidate candidate;
					if (MakeAttackCandidate(branch, button, ActionGuideCategory::ComboBranch, candidate)) {
						// 派生は今の攻撃に続けて出るので、ロックオンやスティックの条件は付かない
						candidate.binding.stick = StickHint::None;
						candidate.binding.withLockOn = false;
						out.push_back(candidate);
					}

					// 同じボタンで、受付の後半に押すと別の技になる派生
					const std::vector<std::string> all = top->GetBranchCandidates(*combat, inputButton);
					const auto it = std::find(all.begin(), all.end(), branch);
					for (auto later = (it == all.end()) ? all.end() : it + 1; later != all.end(); ++later) {
						ActionGuideCandidate wait;
						if (MakeAttackCandidate(*later, button, ActionGuideCategory::BranchLater, wait)) {
							wait.binding.gesture = InputGesture::WaitPress;
							wait.binding.stick = StickHint::None;
							wait.binding.withLockOn = false;
							out.push_back(wait);
						}
					}
					continue;
				}

				// 派生が無いボタンは、別のボタンのルート攻撃で割り込めるか（PlayerCombat::ExecuteCommand と同じ判定）
				if (combat->PeekRootAttackOver(inputButton) != PlayerCombat::RootOverTiming::NotAllowed) {
					CollectRootAttacks(button, ActionGuideCategory::RootInterrupt, out);
				}
			}
		}
	} else {
		for (InputAction button : kAttackButtons) {
			CollectRootAttacks(button, ActionGuideCategory::RootAttack, out);
		}

		if (settings_.showJump) {
			out.push_back({ ActionGuideCatalog::kJump, { InputAction::Jump }, ActionGuideCategory::Jump });
		}
		if (settings_.showLockOn) {
			out.push_back({ ActionGuideCatalog::kLockOn, { InputAction::LockOn, InputGesture::Hold }, ActionGuideCategory::LockOn });
		}
	}

	// 回避は攻撃中でも割り込める（Player::ExecuteCommand が直接処理する）
	if (settings_.showDodge) {
		out.push_back({ ActionGuideCatalog::kDodge, { InputAction::Dodge }, ActionGuideCategory::Dodge });
	}
}

ActionAvailability PlayerActionAvailability::Evaluate(const ActionGuideCandidate& candidate,
	const ActionGuideContext& context, bool& queued) const
{
	queued = false;
	PlayerCombat* combat = player_.GetCombat();
	const PlayerStateAttack* top = combat ? combat->GetCurrentAttackState() : nullptr;
	const InputButton inputButton = ButtonOf(candidate.binding.action);

	switch (candidate.category) {
	case ActionGuideCategory::ChargeRelease:
		return context.isCharging ? ActionAvailability::Executable : ActionAvailability::Unavailable;

	case ActionGuideCategory::ComboBranch:
		if (!top) return ActionAvailability::Unavailable;
		// Cancel に入っていれば押した瞬間に出る。その前に押すと先行入力として覚え、Cancel に入った瞬間に出る
		queued = top->HasBufferedInput() && top->GetBufferedButton() == inputButton;
		return top->IsCancelPhase() ? ActionAvailability::Executable : ActionAvailability::Buffered;

	case ActionGuideCategory::BranchLater:
		// 受付の後半まで待ってから押すと出る。今押すと別の技（ComboBranch の方）になる
		return top ? ActionAvailability::Preview : ActionAvailability::Unavailable;

	case ActionGuideCategory::RootInterrupt:
		if (!combat) return ActionAvailability::Unavailable;
		switch (combat->PeekRootAttackOver(inputButton)) {
		case PlayerCombat::RootOverTiming::Now:
			return ActionAvailability::Executable;
		case PlayerCombat::RootOverTiming::Buffer:
			queued = combat->HasPendingRootAttack() && combat->GetPendingRootButton() == inputButton;
			return ActionAvailability::Buffered;
		default:
			return ActionAvailability::Unavailable;
		}

	case ActionGuideCategory::RootAttack:
	case ActionGuideCategory::Counter:
		// 候補を集めた時点で FindRootAttack（実際に出すときの選び方）が選んだもの
		return ActionAvailability::Executable;

	case ActionGuideCategory::Dodge:
		return player_.CanStartDodge(player_.IsGroundedForActions()) ? ActionAvailability::Executable : ActionAvailability::Unavailable;

	case ActionGuideCategory::Jump: {
		if (context.isAttacking) return ActionAvailability::Unavailable;
		PlayerStateMachine* machine = player_.GetStateMachine();
		const PlayerStateBase* state = machine ? machine->GetCurrentState() : nullptr;
		return (state && state->CanJump()) ? ActionAvailability::Executable : ActionAvailability::Unavailable;
	}

	case ActionGuideCategory::LockOn:
		// 戦闘中で、まだロックオンしていないときだけ
		return (context.isInBattle && !context.isLockOn) ? ActionAvailability::Executable : ActionAvailability::Unavailable;

	default:
		return ActionAvailability::Unavailable;
	}
}
