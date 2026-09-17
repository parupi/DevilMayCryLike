#include "AppEditorWindows.h"
#ifdef _DEBUG

#include <Editor/Core/EditorHost.h>
#include <World3D/Object/Model/Animation/AnimationPlayer.h>

#include "GameData/EnemyCatalog.h"
#include "GameData/GameSession.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Training/TrainingController.h"

#include <imgui/imgui.h>

namespace {

// EnemyCatalog を ImGui のコンボにする。選ばれたインデックスを返す（変わらなければ -1）
int32_t DrawEnemyCombo(const char* label, int32_t currentIndex)
{
	const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
	if (entries.empty()) {
		ImGui::TextDisabled("EnemyCatalog が空です");
		return -1;
	}

	const bool validIndex = (currentIndex >= 0 && currentIndex < static_cast<int32_t>(entries.size()));
	const char* preview = validIndex ? entries[currentIndex].displayName.c_str() : "-";

	int32_t picked = -1;
	if (ImGui::BeginCombo(label, preview)) {
		for (int32_t i = 0; i < static_cast<int32_t>(entries.size()); ++i) {
			const bool selected = (i == currentIndex);
			if (ImGui::Selectable(entries[i].displayName.c_str(), selected)) {
				picked = i;
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("%s\nclass: %s",
					entries[i].description.c_str(), entries[i].className.c_str());
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	return picked;
}

// 起動時にタイトルを飛ばしてトレーニングへ入る設定。トレーニング中でなくても触れる
void DrawBootSection()
{
	if (!ImGui::CollapsingHeader("起動時の行き先", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	GameSession::BootSettings settings = GameSession::GetBootSettings();

	bool changed = ImGui::Checkbox("起動時にトレーニングへ直行する", &settings.bootToTraining);
	ImGui::TextDisabled("敵の調整のたびにタイトルを通らずに済む（Debug のみ・Release では常にタイトル）");

	// 保存されている名前からインデックスを引く
	const std::vector<EnemyCatalogEntry>& entries = EnemyCatalog::GetEntries();
	int32_t index = 0;
	for (int32_t i = 0; i < static_cast<int32_t>(entries.size()); ++i) {
		if (entries[i].className == settings.enemyClassName) {
			index = i;
			break;
		}
	}

	ImGui::BeginDisabled(!settings.bootToTraining);
	const int32_t picked = DrawEnemyCombo("直行したときの相手", index);
	ImGui::EndDisabled();

	if (picked >= 0) {
		settings.enemyClassName = entries[picked].className;
		changed = true;
	}
	// 有効にした直後は名前が空なので、先頭で埋めておく
	if (changed && settings.enemyClassName.empty() && !entries.empty()) {
		settings.enemyClassName = entries[index].className;
	}

	if (changed) {
		// 触った瞬間にファイルへ落とす。次の起動から効く
		GameSession::SetBootSettings(settings);
	}
}

void DrawRuntimeSection(TrainingController& training)
{
	// ゲーム側の設定メニュー。ここと同じ内容をパッドだけで触れるようにしたもの
	if (ImGui::Button("ゲーム画面の設定メニューを開く (TAB)")) {
		training.RequestMenu();
	}

	ImGui::Separator();

	// ── 相手 ──
	const int32_t picked = DrawEnemyCombo("相手", training.GetEnemyIndex());
	if (picked >= 0) {
		training.SelectEnemy(picked);
	}

	if (ImGui::Button("出し直す (F1)")) {
		training.RequestRespawn();
	}
	ImGui::SameLine();
	if (ImGui::Button("リセット (F7)")) {
		training.ResetAll();
	}

	ImGui::Separator();

	// ── スイッチ ──
	bool playerInvincible = training.IsPlayerInvincible();
	if (ImGui::Checkbox("プレイヤー無敵 (F3)", &playerInvincible)) {
		training.SetPlayerInvincible(playerInvincible);
	}

	bool enemyInvincible = training.IsEnemyInvincible();
	if (ImGui::Checkbox("敵無敵 (F4)", &enemyInvincible)) {
		training.SetEnemyInvincible(enemyInvincible);
	}

	bool autoRespawn = training.IsAutoRespawn();
	if (ImGui::Checkbox("倒したら自動で出し直す", &autoRespawn)) {
		training.SetAutoRespawn(autoRespawn);
	}

	bool hudVisible = training.IsHudVisible();
	if (ImGui::Checkbox("画面の状態表示 (F8)", &hudVisible)) {
		training.SetHudVisible(hudVisible);
	}

	ImGui::Text("敵の行動 (F6)");
	for (int32_t i = 0; i < static_cast<int32_t>(TrainingBehavior::Count); ++i) {
		const TrainingBehavior behavior = static_cast<TrainingBehavior>(i);
		if (i > 0) ImGui::SameLine();
		if (ImGui::RadioButton(TrainingController::BehaviorLabel(behavior),
			training.GetBehavior() == behavior)) {
			training.SetBehavior(behavior);
		}
	}
	ImGui::TextDisabled("HOLD=棒立ち / MOVE ONLY=移動のみ / FULL=通常");

	ImGui::Separator();

	// ── いまの相手の状態 ──
	Enemy* enemy = training.GetEnemy();
	if (!enemy) {
		ImGui::TextDisabled("相手を出し直しています…");
		return;
	}

	ImGui::Text("HP    : %.1f / %.1f", enemy->GetHp(), enemy->GetMaxHp());
	ImGui::ProgressBar(enemy->GetHpRatio(), ImVec2(-FLT_MIN, 0.0f));
	ImGui::Text("ステート: %s", enemy->GetCurrentStateName().c_str());
	if (AnimationPlayer* animation = enemy->GetAnimationPlayer()) {
		ImGui::Text("クリップ: %s  (%.2f / %.2f 秒)",
			animation->GetCurrentClipName().c_str(), animation->GetTime(), animation->GetDuration());
	} else {
		ImGui::TextDisabled("クリップ: なし（静的モデル）");
	}
	ImGui::Text("与ダメ : %.1f   ヒット数 %u   直前 %.1f",
		enemy->GetTotalDamageTaken(), enemy->GetDamageHitCount(), enemy->GetLastDamageTaken());
}

} // namespace

void AppEditor::DrawTrainingWindow()
{
	if (!EditorWindow::Begin("Training", EditorWindow::Category::kGame)) {
		return;
	}

	if (TrainingController* training = TrainingController::GetCurrent()) {
		DrawRuntimeSection(*training);
	} else {
		ImGui::TextDisabled("いまはトレーニング中ではありません");
		ImGui::TextDisabled("タイトルの TRAINING から入るか、下の設定で起動時に直行できます");
	}

	ImGui::Separator();
	DrawBootSection();

	EditorWindow::End();
}

#endif // _DEBUG
