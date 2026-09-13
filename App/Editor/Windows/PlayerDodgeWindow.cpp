#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>

#include "GameObject/Character/Player/Player.h"
#include "GameObject/Character/Player/PlayerDodge.h"
#include "GameObject/Character/Player/StateMachine/PlayerStateMachine.h"

#include <imgui/imgui.h>

#include <cmath>

namespace {

constexpr const char* kGroup = PlayerDodgeParams::kGroupName;

GlobalVariables* Global() { return &GlobalVariables::GetInstance(); }

// GlobalVariables の値を直接いじる。Player は毎フレーム Apply() で読み直すので、
// ここで動かした値はその場でゲームへ効く
void DragFloatParam(const char* label, const char* key, float speed, float min, float max, const char* format = "%.3f")
{
	ImGui::DragFloat(label, &Global()->GetValueRef<float>(kGroup, key), speed, min, max, format);
}

} // namespace

void AppEditor::DrawPlayerDodgeWindow()
{
	if (!EditorWindow::Begin("Dodge / Dash", EditorWindow::Category::kCharacter)) {
		return;
	}

	Player* player = FindPlayer();
	if (!player) {
		ImGui::TextDisabled("このシーンに Player はいません");
		EditorWindow::End();
		return;
	}

	const PlayerDodgeParams& params = player->GetDodgeParams();
	const PlayerDodgeRuntime& runtime = player->GetDodgeRuntime();

	// ── 現在の状態 ──
	const PlayerStateBase* state = player->GetStateMachine() ? player->GetStateMachine()->GetCurrentState() : nullptr;
	ImGui::Text("State : %s", state ? state->GetDebugName() : "-");

	const Vector3& velocity = player->GetVelocity();
	const float horizontalSpeed = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
	ImGui::Text("Speed : %.1f  (水平)", horizontalSpeed);
	ImGui::Text("Dodge : %.2f 秒経過 / %.2f", runtime.elapsed, params.dodgeDuration);
	ImGui::Text("無敵 : %s   ジャスト回避 : %s",
		player->IsDodgeInvincible() ? "ON" : "-",
		runtime.justDodgeUsed ? "使用済み" : (runtime.justDodgePending ? "発生待ち" : "-"));
	ImGui::Text("Slow : %s", player->IsJustDodgeSlow() ? "ON" : "-");
	ImGui::Text("カウンター受付 : %s",
		(player->GetCombat() && player->GetCombat()->IsCounterWindowOpen()) ? "受付中" : "-");

	ImGui::Separator();

	// 通常移動との比が一番の目安になるので、倍率も出す。
	// moveSpeed_ は Player 側の定数（10.0f）
	constexpr float kWalkSpeed = 10.0f;
	ImGui::Text("通常移動 %.0f に対して  回避 x%.2f / ダッシュ x%.2f",
		kWalkSpeed, params.dodgeSpeed / kWalkSpeed, params.dashSpeed / kWalkSpeed);
	ImGui::Text("移動距離  回避 %.1f / ダッシュ(最大) %.1f",
		params.dodgeSpeed * params.dodgeDuration, params.dashSpeed * params.dashDuration);

	ImGui::Separator();

	if (ImGui::CollapsingHeader("回避", ImGuiTreeNodeFlags_DefaultOpen)) {
		DragFloatParam("回避の長さ##DodgeDuration", "DodgeDuration", 0.01f, 0.05f, 2.0f, "%.2f 秒");
		DragFloatParam("回避速度##DodgeSpeed", "DodgeSpeed", 0.25f, 0.0f, 60.0f, "%.1f");
		DragFloatParam("無敵時間##DodgeInvincibleTime", "DodgeInvincibleTime", 0.01f, 0.0f, 2.0f, "%.2f 秒");
		DragFloatParam("クールダウン##DodgeCooldown", "DodgeCooldown", 0.01f, 0.0f, 2.0f, "%.2f 秒");
	}

	if (ImGui::CollapsingHeader("ダッシュ", ImGuiTreeNodeFlags_DefaultOpen)) {
		DragFloatParam("ダッシュ速度##DashSpeed", "DashSpeed", 0.25f, 0.0f, 60.0f, "%.1f");
		DragFloatParam("継続時間の上限##DashDuration", "DashDuration", 0.05f, 0.1f, 10.0f, "%.2f 秒");
		DragFloatParam("曲がる速さ##DashTurnRate", "DashTurnRate", 0.1f, 0.0f, 20.0f, "%.1f rad/秒");
		DragFloatParam("入力を離してからの猶予##DashInputGrace", "DashInputGrace", 0.01f, 0.0f, 2.0f, "%.2f 秒");
		DragFloatParam("開始時のFOV変化##DashFovPunch", "DashFovPunch", 0.005f, 0.0f, 0.5f, "%.3f rad");
	}

	if (ImGui::CollapsingHeader("ジャスト回避")) {
		DragFloatParam("スローの長さ##JustDodgeSlowTime", "JustDodgeSlowTime", 0.01f, 0.0f, 1.0f, "%.2f 秒");
		DragFloatParam("スロー中の時間倍率##JustDodgeTimeScale", "JustDodgeTimeScale", 0.01f, 0.0f, 1.0f, "%.2f");
		DragFloatParam("カメラの揺れ##JustDodgeShake", "JustDodgeShake", 0.01f, 0.0f, 1.0f, "%.2f");
		DragFloatParam("無敵の延長##JustDodgeInvincibleAdd", "JustDodgeInvincibleAdd", 0.01f, 0.0f, 2.0f, "%.2f 秒");
		// カウンター攻撃は Attack Editor の「Require Just Dodge (Counter)」を付けた攻撃
		DragFloatParam("カウンター受付時間##CounterWindow", "CounterWindow", 0.01f, 0.0f, 3.0f, "%.2f 秒");
		DragFloatParam("カウンターのクールタイム##CounterCooldown", "CounterCooldown", 0.01f, 0.0f, 10.0f, "%.2f 秒");
	}

	if (ImGui::CollapsingHeader("演出")) {
		DragFloatParam("残像の長さ##TrailLifetime", "TrailLifetime", 0.01f, 0.0f, 2.0f, "%.2f 秒");
	}

	ImGui::Separator();
	if (ImGui::Button("Save##PlayerDodge")) {
		Global()->SaveFile(PlayerDodgeParams::kDirectoryName, kGroup);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Resource/GlobalVariables/%s/%s.json", PlayerDodgeParams::kDirectoryName, kGroup);

	EditorWindow::End();
}

#endif // _DEBUG
