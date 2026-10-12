#include "ActionGuideContext.h"

#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/StateMachine/PlayerStateMachine.h"
#include "GameData/Score/StylishScoreManager.h"
#include "Input/InputContext.h"
#include <World3D/Camera/CameraManager.h>

namespace {
	// ゲーム中のカメラ。これ以外が映っている間は演出中とみなす
	constexpr const char* kGameCameraName = "GameCamera";
}

ActionGuideContext ActionGuideContextBuilder::Build(Player& player, const InputContext* inputContext)
{
	ActionGuideContext context;

	if (PlayerStateMachine* machine = player.GetStateMachine()) {
		if (const PlayerStateBase* state = machine->GetCurrentState()) {
			context.state = state->GetDebugName();
		}
	}

	if (PlayerCombat* combat = player.GetCombat()) {
		context.isAttacking = combat->IsAttacking();
		context.isCharging = combat->IsCharging();
		context.currentAttack = combat->GetCurrentAttackName();
		context.counterWindowOpen = combat->IsCounterWindowOpen();
	}

	context.isGrounded = player.IsGroundedForActions();
	context.isLockOn = player.IsLockOn();
	context.isStunned = (context.state == "Knockback");
	context.isDead = player.IsDying() || context.state == "Death";
	context.isInCutscene = (CameraManager::GetInstance().GetActiveCameraName() != kGameCameraName);
	context.isInputLocked = inputContext ? !inputContext->CanPlayerMove() : false;

	if (StylishScoreManager* score = player.GetScoreManager()) {
		context.isInBattle = score->IsBattleActive();
	}
	return context;
}
