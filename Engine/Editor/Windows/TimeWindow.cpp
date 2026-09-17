#include "TimeWindow.h"
#ifdef _DEBUG

#include "Editor/Core/EditorHost.h"
#include "Utility/DeltaTime.h"
#include "Utility/TimeManager.h"

#include <imgui/imgui.h>

namespace {

void DrawDeltaTimeWindow()
{
	if (!EditorWindow::Begin("DeltaTime", EditorWindow::Category::kEngine)) {
		return;
	}

	ImGui::Text("DeltaTime : %.6f sec", DeltaTime::GetDeltaTime());
	const float unscaled = DeltaTime::GetUnscaledDeltaTime();
	ImGui::Text("実測      : %.6f sec (%.1f FPS)", unscaled, (unscaled > 0.0f) ? 1.0f / unscaled : 0.0f);
	ImGui::Separator();
	ImGui::Text("状態      : %s", DeltaTime::IsPaused() ? "一時停止中" : "再生中");
	ImGui::Text("デバッグ倍率: x%.2f", DeltaTime::GetDebugTimeScale());

	EditorWindow::End();
}

void DrawTimeManagerWindow()
{
	if (!EditorWindow::Begin("TimeManager", EditorWindow::Category::kEngine)) {
		return;
	}

	ImGui::Text("Real  : %.4f sec", TimeManager::GetRealDelta());
	ImGui::Text("Game  : %.4f sec (scale %.2f)", TimeManager::GetGameDelta(), TimeManager::GetGameTimeScale());
	ImGui::Text("VFX   : %.4f sec (scale %.2f)", TimeManager::GetVFXDelta(), TimeManager::GetVFXTimeScale());
	ImGui::Text("UI    : %.4f sec", TimeManager::GetUIDelta());

	float bias = TimeManager::GetVFXBias();
	if (ImGui::SliderFloat("VFXBias", &bias, 0.0f, 1.0f)) {
		TimeManager::SetVFXBias(bias);
	}

	EditorWindow::End();
}

} // namespace

void Editor::DrawTimeWindows()
{
	DrawDeltaTimeWindow();
	DrawTimeManagerWindow();
}

#endif // _DEBUG
