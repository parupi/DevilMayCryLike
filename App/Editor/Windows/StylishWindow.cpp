#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>

#include "GameData/Score/StylishScoreManager.h"
#include "GameObject/Character/Player/Player.h"

#include <imgui/imgui.h>

namespace {

constexpr const char* kGroup = "StylishScore";

GlobalVariables* Global() { return &GlobalVariables::GetInstance(); }

void DragFloatParam(const char* key, float speed, float min, float max)
{
	ImGui::DragFloat(key, &Global()->GetValueRef<float>(kGroup, key), speed, min, max);
}

void DragIntParam(const char* key, float speed, int min, int max)
{
	ImGui::DragInt(key, &Global()->GetValueRef<int32_t>(kGroup, key), speed, min, max);
}

} // namespace

void AppEditor::DrawStylishWindow()
{
	if (!EditorWindow::Begin("Stylish", EditorWindow::Category::kScore)) {
		return;
	}

	Player* player = FindPlayer();
	StylishScoreManager* score = player ? player->GetScoreManager() : nullptr;
	if (!score) {
		ImGui::TextDisabled("このシーンに Player はいません");
		EditorWindow::End();
		return;
	}

	const StylishScoreManager::EditorStatus status = score->MakeEditorStatus();

	ImGui::Text("Rank : %s (%s)", score->GetCurrentRank().c_str(), score->GetRankDisplayName());
	ImGui::Text("Style Point : %d", score->GetCurrentScore());
	ImGui::Text("Battle : %s%s   Peak : %d",
		status.battleActive ? "ACTIVE" : "-",
		status.battleActive ? (status.battleForced ? " (forced)" : " (auto)") : "",
		static_cast<int32_t>(status.battlePeak));
	ImGui::Text("Battles : %zu   Final(avg) : %d", status.battleCount, score->GetFinalScore());
	ImGui::Text("NearestEnemy : %.1f (range %.1f)   Disengage : %.2f",
		status.nearestEnemyDist, status.battleRange, status.disengageTimer);
	ImGui::Text("DecayHold : %s (%.2f)",
		status.holdingAtBoundary ? "HOLD" : "-", status.boundaryHoldTimer);
	ImGui::Separator();
	ImGui::Text("Combo   : %u  (x%.2f)", status.comboCount, status.comboMultiplier);
	ImGui::Text("Repeat  : %s x%u  (x%.2f)",
		status.lastAttackName.c_str(), status.repeatCount, status.repeatPenalty);
	ImGui::Text("Variety : %zu  (x%.2f)", status.varietyCount, status.diversityBonus);

	ImGui::Separator();
	ImGui::TextUnformatted("Test:");
	ImGui::SameLine();
	if (ImGui::Button("Hit")) { score->OnAttackHit({ "TestSlash", false, false, 1.0f }); }
	ImGui::SameLine();
	if (ImGui::Button("Air")) { score->OnAttackHit({ "TestAir", true, false, 1.0f }); }
	ImGui::SameLine();
	if (ImGui::Button("Kill")) { score->OnEnemyKilled(1.0f); }
	ImGui::SameLine();
	if (ImGui::Button("Damage")) { score->OnDamage(); }
	if (ImGui::Button("Begin Battle")) { score->BeginBattle(); }
	ImGui::SameLine();
	if (ImGui::Button("End Battle")) { score->EndBattle(); }

	if (ImGui::CollapsingHeader("Params")) {
		DragFloatParam("PointHit", 1.0f, 0.0f, 1000.0f);
		DragFloatParam("PointStrong", 1.0f, 0.0f, 1000.0f);
		DragFloatParam("PointAirCombo", 1.0f, 0.0f, 1000.0f);
		DragFloatParam("PointKill", 1.0f, 0.0f, 1000.0f);
		DragFloatParam("ComboWindow", 0.05f, 0.1f, 10.0f);
		DragFloatParam("ComboMulMax", 0.05f, 1.0f, 10.0f);
		DragIntParam("ComboMulHitsForMax", 1.0f, 1, 200);
		DragFloatParam("RepeatPenaltyStep", 0.01f, 0.0f, 1.0f);
		DragFloatParam("RepeatPenaltyMin", 0.01f, 0.0f, 1.0f);
		DragFloatParam("DiversityBonusPer", 0.01f, 0.0f, 2.0f);
		DragIntParam("DiversityMaxStacks", 1.0f, 0, 20);
		DragFloatParam("DamagePenaltyScale", 0.01f, 0.0f, 1.0f);
		ImGui::Checkbox("AutoBattleEnabled", &Global()->GetValueRef<bool>(kGroup, "AutoBattleEnabled"));
		DragFloatParam("BattleRange", 0.5f, 0.0f, 200.0f);
		DragFloatParam("AutoBattleEndTime", 0.1f, 0.0f, 30.0f);
		DragFloatParam("AutoBattleMinPeak", 5.0f, 0.0f, 5000.0f);
		DragFloatParam("DecayIdleTime", 0.05f, 0.0f, 20.0f);
		DragFloatParam("DecaySpeed", 1.0f, 0.0f, 2000.0f);
		DragFloatParam("BoundaryHoldTime", 0.05f, 0.0f, 10.0f);
		DragFloatParam("RankC", 10.0f, 0.0f, 10000.0f);
		DragFloatParam("RankB", 10.0f, 0.0f, 10000.0f);
		DragFloatParam("RankA", 10.0f, 0.0f, 10000.0f);
		DragFloatParam("RankS", 10.0f, 0.0f, 10000.0f);
		DragFloatParam("RankSS", 10.0f, 0.0f, 10000.0f);
		DragFloatParam("RankSSS", 10.0f, 0.0f, 10000.0f);
		if (ImGui::Button("Save##Score")) {
			Global()->SaveFile("Score", kGroup);
		}
	}

	EditorWindow::End();
}

#endif // _DEBUG
