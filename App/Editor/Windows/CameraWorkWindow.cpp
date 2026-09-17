#include "AppEditorWindows.h"
#ifdef _DEBUG

#include "Editor/AppEditor.h"

#include <Debugger/GlobalVariables.h>
#include <Editor/Core/EditorHost.h>

#include "GameObject/Camera/GameCamera.h"

#include <imgui/imgui.h>
#include <string>

namespace {

// パラメータは GlobalVariables のカメラ名グループに入っている。
// キーを直接叩けるので、GameCamera 側の内部変数を公開しなくてよい
GlobalVariables* Global() { return &GlobalVariables::GetInstance(); }

void DragFloatParam(const std::string& group, const char* key, float speed, float min, float max)
{
	ImGui::DragFloat(key, &Global()->GetValueRef<float>(group, key), speed, min, max);
}

void CheckboxParam(const std::string& group, const char* key)
{
	ImGui::Checkbox(key, &Global()->GetValueRef<bool>(group, key));
}

} // namespace

void AppEditor::DrawCameraWorkWindow()
{
	if (!EditorWindow::Begin("GameCamera", EditorWindow::Category::kCamera)) {
		return;
	}

	GameCamera* camera = FindGameCamera();
	if (!camera) {
		ImGui::TextDisabled("このシーンに GameCamera はいません");
		EditorWindow::End();
		return;
	}

	const std::string& group = camera->name_;
	const GameCamera::EditorStatus status = camera->MakeEditorStatus();

	ImGui::Text("Mode: %s   State: %s (%.2f)",
		status.mode == GameCamera::Mode::LockOn ? "LockOn" : "Free",
		status.state == GameCamera::State::Battle ? "Battle" : "Normal",
		status.battleBlend);

	if (ImGui::CollapsingHeader("Follow / Orbit", ImGuiTreeNodeFlags_DefaultOpen)) {
		// "Distance" は基準距離。実行時距離はこれへ滑らかに寄っていく
		DragFloatParam(group, "Distance", 0.1f, 1.0f, 100.0f);
		DragFloatParam(group, "DistanceLerpSpeed", 0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "BaseHeight", 0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "MinDistance", 0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "SensitivityX", 0.001f, 0.0f, 1.0f);
		DragFloatParam(group, "SensitivityY", 0.001f, 0.0f, 1.0f);
		DragFloatParam(group, "PitchLimit", 0.01f, 0.0f, 1.5f);
	}

	if (ImGui::CollapsingHeader("Auto Rotation")) {
		CheckboxParam(group, "AutoRotateEnabled");
		DragFloatParam(group, "AutoRotateDelay", 0.05f, 0.0f, 5.0f);
		DragFloatParam(group, "AutoRotateSpeed", 0.05f, 0.1f, 20.0f);
		DragFloatParam(group, "AutoRotateMoveSpeed", 0.05f, 0.0f, 20.0f);
	}

	if (ImGui::CollapsingHeader("Lag / LookAt", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 1でプレイヤーの移動をそのままカメラへ渡し、追従の定常遅れを消す。
		// プレイヤーの位置の細かい震えが気になる時だけ下げる
		DragFloatParam(group, "FollowFeedForward", 0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "PositionLagSpeed", 0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "LookLagSpeed", 0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "LookForwardOffset", 0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookHeight", 0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookPitchDip", 0.05f, 0.0f, 20.0f);
		DragFloatParam(group, "LookTargetLagSpeed", 0.05f, 0.1f, 30.0f);
	}

	if (ImGui::CollapsingHeader("Collision")) {
		DragFloatParam(group, "CollisionMargin", 0.01f, 0.0f, 5.0f);
		DragFloatParam(group, "CollisionMinDist", 0.01f, 0.0f, 10.0f);
		// カメラの当たり半径。角をかすめた時のバタつき止め
		DragFloatParam(group, "CollisionRadius", 0.01f, 0.0f, 3.0f);
		// 遮蔽されたら速く寄り、晴れたらゆっくり戻す
		DragFloatParam(group, "CollisionInSpeed", 0.5f, 0.1f, 100.0f);
		DragFloatParam(group, "CollisionOutSpeed", 0.05f, 0.1f, 30.0f);
		ImGui::Text("Ratio: %.2f", status.collisionRatio);
	}

	if (ImGui::CollapsingHeader("Framing (LockOn)", ImGuiTreeNodeFlags_DefaultOpen)) {
		// プレイヤーと敵の両方が安全枠に収まるまでカメラを下げる
		CheckboxParam(group, "FramingEnabled");
		DragFloatParam(group, "FramingSafeRatio", 0.01f, 0.2f, 1.0f);
		DragFloatParam(group, "FramingLookWeight", 0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "FramingMaxLookOffset", 0.1f, 0.0f, 30.0f);
		DragFloatParam(group, "FramingMaxDistanceAdd", 0.1f, 0.0f, 40.0f);
		ImGui::Separator();
		// プレイヤーを必ず画面内に残す最終保証（FramingSafeRatioより外側にすること）
		CheckboxParam(group, "FramingSafetyEnabled");
		DragFloatParam(group, "FramingClampRatio", 0.01f, 0.2f, 1.0f);
		ImGui::Text("Distance: %.2f", status.distance);
	}

	if (ImGui::CollapsingHeader("LockOn")) {
		DragFloatParam(group, "LockOnDistanceMul", 0.01f, 0.0f, 5.0f);
		DragFloatParam(group, "LockOnDistanceMin", 0.1f, 0.0f, 100.0f);
		DragFloatParam(group, "LockOnDistanceMax", 0.1f, 0.0f, 100.0f);
		DragFloatParam(group, "LockOnHeight", 0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "LockOnRightOffset", 0.1f, -20.0f, 20.0f);
		DragFloatParam(group, "LockOnLagSpeed", 0.05f, 0.1f, 30.0f);
		// 敵の真下・真上を通った時の暴れ止め。DeadZone内では方位を凍結する
		DragFloatParam(group, "LockOnYawSpeed", 0.1f, 0.1f, 60.0f);
		DragFloatParam(group, "LockOnYawMaxSpeed", 0.1f, 0.1f, 20.0f);
		DragFloatParam(group, "LockOnYawDeadZone", 0.05f, 0.0f, 20.0f);
	}

	if (ImGui::CollapsingHeader("FOV / Action")) {
		DragFloatParam(group, "FovNormal", 0.005f, 0.1f, 2.0f);
		DragFloatParam(group, "FovDash", 0.005f, 0.1f, 2.0f);
		DragFloatParam(group, "FovSpeedMin", 0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "FovSpeedMax", 0.1f, 0.0f, 50.0f);
		DragFloatParam(group, "FovLerpSpeed", 0.05f, 0.1f, 30.0f);
		DragFloatParam(group, "AttackDistanceScale", 0.01f, 0.3f, 1.0f);
		DragFloatParam(group, "ActionZoomSpeed", 0.05f, 0.1f, 30.0f);
		ImGui::Text("ZoomScale: %.2f  FOV: %.3f", status.actionZoomScale, camera->GetFovY());
	}

	if (ImGui::CollapsingHeader("Enemy Framing")) {
		CheckboxParam(group, "EnemyFramingEnabled");
		DragFloatParam(group, "EnemyFramingEdge", 0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "EnemyFramingYawSpeed", 0.02f, 0.0f, 5.0f);
	}

	if (ImGui::CollapsingHeader("Camera State (Battle)")) {
		CheckboxParam(group, "BattleStateEnabled");
		DragFloatParam(group, "BattleDistanceScale", 0.01f, 0.3f, 1.5f);
		DragFloatParam(group, "BattleFovAdd", 0.005f, -0.5f, 0.5f);
		DragFloatParam(group, "BattleBlendSpeed", 0.05f, 0.1f, 20.0f);
	}

	if (ImGui::CollapsingHeader("Shake")) {
		DragFloatParam(group, "ShakeMaxOffset", 0.05f, 0.0f, 10.0f);
		DragFloatParam(group, "ShakeDecayRate", 0.05f, 0.1f, 20.0f);
		DragFloatParam(group, "ShakeHitTrauma", 0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "ShakeLandTrauma", 0.01f, 0.0f, 1.0f);
		DragFloatParam(group, "ShakeLandSpeedThreshold", 0.1f, 0.0f, 50.0f);
		ImGui::Text("Trauma: %.2f", status.shakeTrauma);
		if (ImGui::Button("Test Shake")) {
			camera->AddShake(0.6f);
		}
	}

	if (ImGui::Button("Save##Camera")) {
		Global()->SaveFile("Camera", group);
	}

	EditorWindow::End();
}

#endif // _DEBUG
