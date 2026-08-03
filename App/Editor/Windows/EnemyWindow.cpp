#include "AppEditorWindows.h"
#ifdef _DEBUG

#include <Editor/Core/EditorHost.h>
#include <World3D/Object/Object3d.h>
#include <World3D/Object/Object3dManager.h>
#include <World3D/WorldTransform.h>

#include "GameObject/Character/Enemy/BossKnight/BossKnight.h"
#include "GameObject/Character/Enemy/Enemy.h"

#include <imgui/imgui.h>

void AppEditor::DrawEnemyWindow()
{
	if (!EditorWindow::Begin("Enemy", EditorWindow::Category::kCharacter)) {
		return;
	}

	Object3dManager* objects = Editor::Ctx().object3dManager;
	if (!objects) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	// もとは各敵クラスの DebugGui() が自分のウィンドウを開いていた。
	// 種類ごとに窓が増えても仕方ないので、1つにまとめて一覧にする
	int count = 0;
	if (ImGui::BeginTable("##enemies", 4,
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
		ImGui::TableSetupColumn("名前", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 110.0f);
		ImGui::TableSetupColumn("状態", ImGuiTableColumnFlags_WidthFixed, 70.0f);
		ImGui::TableSetupColumn("位置", ImGuiTableColumnFlags_WidthFixed, 150.0f);
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (Object3d* object : objects->GetAllObject()) {
			auto* enemy = dynamic_cast<Enemy*>(object);
			if (!enemy) {
				continue;
			}
			++count;
			ImGui::PushID(enemy);
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(enemy->name_.c_str());
			// ボスだけフェーズを併記する
			if (auto* boss = dynamic_cast<BossKnight*>(enemy)) {
				const float hp = boss->GetHp();
				const int phase = (hp > BossKnight::kMaxHp * 0.66f) ? 1
					: (hp > BossKnight::kMaxHp * 0.33f) ? 2 : 3;
				ImGui::SameLine();
				ImGui::TextDisabled("[BOSS ph%d]", phase);
			}

			ImGui::TableNextColumn();
			ImGui::ProgressBar(enemy->GetHpRatio(), ImVec2(-FLT_MIN, 0.0f));
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%.1f", enemy->GetHp());
			}

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(enemy->IsActive() ? "Active" : "-");

			ImGui::TableNextColumn();
			const Vector3& position = enemy->GetWorldTransform()->GetTranslation();
			ImGui::Text("%.1f, %.1f, %.1f", position.x, position.y, position.z);

			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	if (count == 0) {
		ImGui::TextDisabled("(このシーンに敵はいません)");
	}

	EditorWindow::End();
}

#endif // _DEBUG
