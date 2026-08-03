#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Editor/Core/EditorHost.h>

#include "GameObject/Character/Player/Combat/PlayerCombat.h"
#include "GameObject/Character/Player/Player.h"

#include <imgui/imgui.h>

void AppEditor::DrawAttackEditorWindows()
{
	Player* player = FindPlayer();
	PlayerCombat* combat = player ? player->GetCombat() : nullptr;

	// もとは PlayerCombat::Update() の中から自分で開いていた2つのウィンドウ。
	// 中身は攻撃グラフそのものなので PlayerCombat 側に残し、
	// ここは「いつ・どのウィンドウとして出すか」だけを持つ
	if (EditorWindow::Begin("Attack Editor", EditorWindow::Category::kCharacter)) {
		if (combat) {
			combat->DrawAttackDataEditorUI();
		} else {
			ImGui::TextDisabled("このシーンに Player はいません");
		}
		EditorWindow::End();
	}

	if (EditorWindow::Begin("Attack Derivative Editor", EditorWindow::Category::kCharacter)) {
		if (combat) {
			combat->DrawAttackDerivativeEditorUI();
		} else {
			ImGui::TextDisabled("このシーンに Player はいません");
		}
		EditorWindow::End();
	}
}

#endif // _DEBUG
