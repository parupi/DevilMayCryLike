#include "CameraWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorCamera.h"
#include "Editor/Core/EditorHost.h"
#include "Editor/Core/EditorMenuBar.h"

#include "World3D/Camera/BaseCamera.h"
#include "World3D/Camera/CameraManager.h"

#include <imgui/imgui.h>
#include <numbers>
#include <string>

namespace {

std::string g_selectedCamera;
float g_transitionTime = 0.0f;

} // namespace

void Editor::DrawCameraWindow()
{
	if (!EditorWindow::Begin("Camera", EditorWindow::Category::kCamera)) {
		return;
	}

	CameraManager* manager = Ctx().cameraManager;
	if (!manager) {
		ImGui::TextDisabled("EditorContext がまだ設定されていません");
		EditorWindow::End();
		return;
	}

	if (ImGui::CollapsingHeader("デバッグカメラ", ImGuiTreeNodeFlags_DefaultOpen)) {
		EditorCamera::DrawSettings();
	}
	ImGui::Separator();

	const std::vector<std::string> names = manager->GetCameraNames();
	const std::string& activeName = manager->GetActiveCameraName();

	ImGui::Text("アクティブ: %s", activeName.empty() ? "(未設定)" : activeName.c_str());
	if (EditorCamera::IsActive()) {
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "(デバッグカメラが割り込み中)");
	}
	if (manager->IsTransition()) {
		ImGui::SameLine();
		ImGui::TextDisabled("(切り替え中)");
	}
	ImGui::Separator();

	if (names.empty()) {
		ImGui::TextDisabled("(カメラがありません)");
		EditorWindow::End();
		return;
	}

	// 選択が消えていたらアクティブへ寄せる
	bool selectedExists = false;
	for (const std::string& name : names) {
		if (name == g_selectedCamera) {
			selectedExists = true;
			break;
		}
	}
	if (!selectedExists) {
		g_selectedCamera = activeName;
	}

	if (ImGui::BeginChild("##cameraList", ImVec2(0.0f, 110.0f), ImGuiChildFlags_Borders)) {
		for (const std::string& name : names) {
			ImGui::PushID(name.c_str());
			if (ImGui::Selectable(name.c_str(), name == g_selectedCamera)) {
				g_selectedCamera = name;
			}
			if (name == activeName) {
				ImGui::SameLine();
				ImGui::TextDisabled("  ← アクティブ");
			}
			ImGui::PopID();
		}
	}
	ImGui::EndChild();

	ImGui::SetNextItemWidth(120.0f);
	ImGui::DragFloat("補間時間(秒)", &g_transitionTime, 0.01f, 0.0f, 5.0f);
	ImGui::SameLine();
	ImGui::BeginDisabled(g_selectedCamera.empty() || g_selectedCamera == activeName || manager->IsTransition());
	if (ImGui::Button("このカメラに切り替え")) {
		manager->SetActiveCamera(g_selectedCamera, g_transitionTime);
		EditorMenuBar::ShowToast("%s に切り替えました", g_selectedCamera.c_str());
	}
	ImGui::EndDisabled();

	ImGui::Separator();

	BaseCamera* camera = g_selectedCamera.empty() ? nullptr : manager->FindCamera(g_selectedCamera);
	if (!camera) {
		ImGui::TextDisabled("カメラを選んでください");
		EditorWindow::End();
		return;
	}

	ImGui::TextUnformatted(camera->name_.c_str());

	// ゲーム側がカメラを毎フレーム動かしている場合、ここで編集しても上書きされる。
	// それでも「今どこを向いているか」を読むのに役立つので出しておく
	ImGui::DragFloat3("位置", &camera->GetTranslate().x, 0.05f);
	ImGui::DragFloat3("回転(rad)", &camera->GetRotate().x, 0.01f);

	float fov = camera->GetFovY();
	if (ImGui::SliderFloat("FOV(rad)", &fov, 0.1f, std::numbers::pi_v<float> -0.1f)) {
		camera->SetFovY(fov);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("%.1f°", fov * 180.0f / std::numbers::pi_v<float>);

	float nearClip = camera->GetNearClip();
	if (ImGui::DragFloat("near", &nearClip, 0.01f, 0.001f, 100.0f)) {
		camera->SetNearClip(nearClip);
	}
	float farClip = camera->GetFarClip();
	if (ImGui::DragFloat("far", &farClip, 1.0f, 1.0f, 10000.0f)) {
		camera->SetFarClip(farClip);
	}

	ImGui::TextDisabled("アスペクト比 %.3f", camera->GetAspectRate());

	const Vector3 forward = camera->GetForward();
	ImGui::TextDisabled("前方向 %.2f, %.2f, %.2f", forward.x, forward.y, forward.z);

	EditorWindow::End();
}

#endif // _DEBUG
