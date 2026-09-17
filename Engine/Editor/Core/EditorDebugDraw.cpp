#include "EditorDebugDraw.h"
#ifdef _DEBUG

#include "Debugger/GlobalVariables.h"
#include "World3D/Primitive/PrimitiveLineDrawer.h"
#include <imgui/imgui.h>

namespace {

struct FlagInfo {
	const char* key;    // json のキー
	const char* label;  // メニューに出す名前
	bool enabled;
};

// Flag の並びと必ず一致させること
FlagInfo g_flags[static_cast<int>(EditorDebugDraw::Flag::Count)] = {
	{ "Collider",    "コライダー",   false },
	{ "LightGizmo",  "ライト",       false },
	{ "AttackTrail", "攻撃の軌跡",   false },
	{ "Grid",        "グリッド",     false },
};

// グリッドの見た目
float g_gridExtent = 20.0f;   // 中心からの片側の長さ
float g_gridSpacing = 1.0f;   // 線の間隔

constexpr const char* kDirectoryName = "Editor";
constexpr const char* kGroupName = "EditorDebugDraw";

FlagInfo& Info(EditorDebugDraw::Flag flag)
{
	return g_flags[static_cast<int>(flag)];
}

} // namespace

bool EditorDebugDraw::IsEnabled(Flag flag)
{
	return Info(flag).enabled;
}

void EditorDebugDraw::SetEnabled(Flag flag, bool enabled)
{
	Info(flag).enabled = enabled;
}

void EditorDebugDraw::DrawMenu()
{
	for (auto& info : g_flags) {
		ImGui::MenuItem(info.label, nullptr, &info.enabled);
	}

	ImGui::Separator();
	if (ImGui::BeginMenu("グリッド設定")) {
		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat("範囲", &g_gridExtent, 1.0f, 1.0f, 500.0f, "%.0f m");
		ImGui::SetNextItemWidth(140.0f);
		ImGui::DragFloat("間隔", &g_gridSpacing, 0.1f, 0.1f, 50.0f, "%.1f m");
		ImGui::EndMenu();
	}

	ImGui::Separator();
	if (ImGui::MenuItem("すべて表示")) {
		for (auto& info : g_flags) {
			info.enabled = true;
		}
	}
	if (ImGui::MenuItem("すべて非表示")) {
		for (auto& info : g_flags) {
			info.enabled = false;
		}
	}
	if (ImGui::MenuItem("設定を保存")) {
		SaveSettings();
	}
}

void EditorDebugDraw::DrawGrid()
{
	if (!IsEnabled(Flag::Grid)) {
		return;
	}
	if (g_gridSpacing <= 0.0f || g_gridExtent <= 0.0f) {
		return;
	}

	auto& drawer = PrimitiveLineDrawer::GetInstance();

	const Vector4 lineColor{ 0.35f, 0.35f, 0.4f, 1.0f };
	const Vector4 axisX{ 0.8f, 0.25f, 0.25f, 1.0f };
	const Vector4 axisZ{ 0.25f, 0.45f, 0.85f, 1.0f };

	const int half = static_cast<int>(g_gridExtent / g_gridSpacing);
	for (int i = -half; i <= half; ++i) {
		const float offset = static_cast<float>(i) * g_gridSpacing;

		// Z方向に伸びる線（＝X座標が offset）
		drawer.DrawLine({ offset, 0.0f, -g_gridExtent }, { offset, 0.0f, g_gridExtent },
			(i == 0) ? axisZ : lineColor);
		// X方向に伸びる線
		drawer.DrawLine({ -g_gridExtent, 0.0f, offset }, { g_gridExtent, 0.0f, offset },
			(i == 0) ? axisX : lineColor);
	}
}

void EditorDebugDraw::LoadSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	gv.LoadFile(kDirectoryName, kGroupName);

	for (auto& info : g_flags) {
		if (gv.HasItem(kGroupName, info.key)) {
			info.enabled = gv.GetValueRef<bool>(kGroupName, info.key);
		}
	}
	if (gv.HasItem(kGroupName, "GridExtent")) {
		g_gridExtent = gv.GetValueRef<float>(kGroupName, "GridExtent");
	}
	if (gv.HasItem(kGroupName, "GridSpacing")) {
		g_gridSpacing = gv.GetValueRef<float>(kGroupName, "GridSpacing");
	}
}

void EditorDebugDraw::SaveSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);

	for (const auto& info : g_flags) {
		gv.SetValue(kGroupName, info.key, info.enabled);
	}
	gv.SetValue(kGroupName, "GridExtent", g_gridExtent);
	gv.SetValue(kGroupName, "GridSpacing", g_gridSpacing);

	gv.SaveFile(kDirectoryName, kGroupName);
}

#endif // _DEBUG
