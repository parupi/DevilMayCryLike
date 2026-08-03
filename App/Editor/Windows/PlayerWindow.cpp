#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Editor/Core/EditorHost.h>
#include <World3D/WorldTransform.h>

#include "GameObject/Character/Player/Combat/AttackPlayer.h"
#include "GameObject/Character/Player/Combat/PlayerCombat.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/State/PlayerStateBase.h"
#include "GameObject/Character/Player/StateMachine/PlayerStateMachine.h"

#include <imgui/imgui.h>

void AppEditor::DrawPlayerWindow()
{
	if (!EditorWindow::Begin("Player", EditorWindow::Category::kCharacter)) {
		return;
	}

	Player* player = FindPlayer();
	if (!player) {
		ImGui::TextDisabled("このシーンに Player はいません");
		EditorWindow::End();
		return;
	}

	// もとは PlayerStateMachine::DebugGui() が出していた
	if (PlayerStateMachine* stateMachine = player->GetStateMachine()) {
		if (PlayerStateBase* state = stateMachine->GetCurrentState()) {
			ImGui::Text("State : %s", state->GetDebugName());
		}
	}
	ImGui::Text("HP    : %d", player->GetHp());
	ImGui::Text("接地  : %s", player->GetOnGround() ? "true" : "false");

	const Vector3& velocity = player->GetVelocity();
	ImGui::Text("速度  : %.2f, %.2f, %.2f", velocity.x, velocity.y, velocity.z);

	if (PlayerCombat* combat = player->GetCombat()) {
		const std::string& attackName = combat->GetCurrentAttackName();
		ImGui::Text("攻撃  : %s", attackName.empty() ? "-" : attackName.c_str());
	}

	const Vector3& position = player->GetWorldTransform()->GetTranslation();
	ImGui::TextDisabled("位置 %.2f, %.2f, %.2f", position.x, position.y, position.z);
	ImGui::TextDisabled("Transform などは Inspector から編集できます");

	EditorWindow::End();

	// --- 攻撃プレビュー（もとは AttackPlayer::DrawImGui が自前で開いていた） ---
	if (!EditorWindow::Begin("Attack Player", EditorWindow::Category::kCharacter)) {
		return;
	}
	if (PlayerCombat* combat = player->GetCombat()) {
		if (AttackPlayer* attackPlayer = combat->GetAttackPlayer()) {
			attackPlayer->DrawEditorContents();
		} else {
			ImGui::TextDisabled("AttackPlayer がありません");
		}
	}
	EditorWindow::End();
}

#endif // _DEBUG
