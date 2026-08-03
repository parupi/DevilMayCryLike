#include "EditorLayout.h"
#ifdef _DEBUG

#include "EditorWindowRegistry.h"
#include <imgui/imgui_internal.h>
#include <utility>

namespace {

// 登録順にLayoutメニューへ並ぶ。先頭はエンジン標準（「配置をリセット」の飛び先）
std::vector<Editor::LayoutPreset> g_presets;

// 予約中のプリセット。-1 なら何もしない
int g_pendingPreset = -1;

// 指定ノードへウィンドウ群をドッキングし、まとめて表示状態にする。
// DockBuilderDockWindow はまだ存在しないウィンドウにも予約できるので、
// そのシーンに出てこないウィンドウが混じっていても問題ない
void DockAll(const std::vector<std::string>& windows, ImGuiID nodeId)
{
	for (const std::string& name : windows) {
		ImGui::DockBuilderDockWindow(name.c_str(), nodeId);
		EditorWindow::SetVisible(name.c_str(), true);
	}
}

} // namespace

void Editor::AddLayoutPreset(LayoutPreset preset)
{
	g_presets.push_back(std::move(preset));
}

void EditorLayout::RegisterBuiltinPresets()
{
	// エンジンだけで完結する配置。
	// アプリ固有のプリセット（バトル調整・カメラ調整など）は App 側から
	// Editor::AddLayoutPreset() で足すこと。エンジンはアプリのウィンドウ名を知らない
	Editor::AddLayoutPreset({
		"標準",
		{ "Game" },
		{ "Hierarchy" },
		{ "Inspector", "Light Manager" },
		{ "Debug Log", "TimeManager", "DeltaTime" },
		0.18f, 0.24f, 0.26f,
		});

	Editor::AddLayoutPreset({
		"アセット",
		{ "Game" },
		{ "Hierarchy" },
		{ "Inspector" },
		{ "Asset Browser", "Audio", "Debug Log" },
		0.18f, 0.24f, 0.34f,
		});

	Editor::AddLayoutPreset({
		"描画・ライト",
		{ "Game" },
		{ "Hierarchy" },
		{ "Render", "Light Manager", "Camera" },
		{ "Shadow Map (CSM)", "Profiler", "Debug Log" },
		0.16f, 0.26f, 0.30f,
		});

	Editor::AddLayoutPreset({
		"VFX作業",
		{ "Game" },
		{ "Particle Groups" },
		{ "Emitters", "VFX" },
		{ "Particle Node Graph", "Debug Log" },
		0.18f, 0.26f, 0.38f,
		});
}

void EditorLayout::RequestPreset(int index)
{
	if (index < 0 || index >= static_cast<int>(g_presets.size())) {
		return;
	}
	g_pendingPreset = index;
}

void EditorLayout::ApplyPendingPreset(ImGuiID dockspaceId)
{
	if (g_pendingPreset < 0 || g_pendingPreset >= static_cast<int>(g_presets.size())) {
		g_pendingPreset = -1;
		return;
	}
	const Editor::LayoutPreset& def = g_presets[g_pendingPreset];
	g_pendingPreset = -1;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();

	// 既存のノードを捨てて組み直す
	ImGui::DockBuilderRemoveNode(dockspaceId);
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	// 分割の前にサイズを与えないと、比率どおりに割れない
	ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

	ImGuiID center = dockspaceId;
	ImGuiID left = 0;
	ImGuiID right = 0;
	ImGuiID bottom = 0;

	if (!def.left.empty() && def.leftRatio > 0.0f) {
		left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, def.leftRatio, nullptr, &center);
	}
	if (!def.right.empty() && def.rightRatio > 0.0f) {
		right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, def.rightRatio, nullptr, &center);
	}
	if (!def.bottom.empty() && def.bottomRatio > 0.0f) {
		bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, def.bottomRatio, nullptr, &center);
	}

	DockAll(def.center, center);
	if (left) {
		DockAll(def.left, left);
	}
	if (right) {
		DockAll(def.right, right);
	}
	if (bottom) {
		DockAll(def.bottom, bottom);
	}

	ImGui::DockBuilderFinish(dockspaceId);
}

void EditorLayout::DrawMenu()
{
	if (g_presets.empty()) {
		ImGui::TextDisabled("(プリセットが登録されていません)");
		return;
	}

	for (int i = 0; i < static_cast<int>(g_presets.size()); ++i) {
		if (ImGui::MenuItem(g_presets[i].name.c_str())) {
			RequestPreset(i);
		}
	}

	ImGui::Separator();
	if (ImGui::MenuItem("配置をリセット")) {
		RequestPreset(0);
	}
	if (ImGui::MenuItem("imgui.ini を今すぐ保存")) {
		ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
	}
}

#endif // _DEBUG
