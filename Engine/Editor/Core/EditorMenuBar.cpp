#include "EditorMenuBar.h"
#ifdef _DEBUG

#include "EditorDebugDraw.h"
#include "EditorGizmo.h"
#include "EditorHost.h"
#include "EditorLayout.h"
#include "EditorPicking.h"
#include "EditorStats.h"
#include "EditorWindowRegistry.h"
#include "Debugger/GlobalVariables.h"
#include "Scene/SceneManager.h"
#include "Utility/DeltaTime.h"

#include <cstdarg>
#include <cstdio>
#include <imgui/imgui.h>
#include <string>

namespace {

std::string g_toastText;
float g_toastTimer = 0.0f;
constexpr float kToastDuration = 2.5f;

// --- 各メニュー ---

void SaveEditorSettings()
{
	EditorWindow::SaveSettings();
	EditorDebugDraw::SaveSettings();
	EditorGizmo::SaveSettings();
	EditorPicking::SaveSettings();
	ImGui::SaveIniSettingsToDisk(ImGui::GetIO().IniFilename);
}

void DrawFileMenu()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();

	if (ImGui::MenuItem("全パラメータを保存", "Ctrl+S")) {
		const size_t count = gv.SaveAllFiles();
		EditorMenuBar::ShowToast("%zu グループを保存しました", count);
	}
	if (ImGui::MenuItem("全パラメータを再読込")) {
		const size_t count = gv.ReloadAllFiles();
		EditorMenuBar::ShowToast("%zu グループを読み直しました", count);
	}

	if (ImGui::BeginMenu("保存先の一覧")) {
		const auto& directories = gv.GetGroupDirectories();
		if (directories.empty()) {
			ImGui::TextDisabled("(まだ何も読み書きしていません)");
		}
		for (const auto& [groupName, directoryName] : directories) {
			ImGui::TextDisabled("%s  →  %s/", groupName.c_str(), directoryName.c_str());
		}
		ImGui::EndMenu();
	}

	ImGui::Separator();

	if (ImGui::MenuItem("エディタ設定を保存")) {
		SaveEditorSettings();
		EditorMenuBar::ShowToast("エディタ設定を保存しました");
	}
}

void DrawSceneMenu()
{
	SceneManager& sceneManager = SceneManager::GetInstance();
	const std::string& current = sceneManager.GetCurrentSceneName();
	// 切り替え予約が入っている間は二重予約でassertするので、メニュー側で止める
	const bool canChange = !sceneManager.IsSceneChangePending();

	ImGui::TextDisabled("現在: %s", current.empty() ? "(未設定)" : current.c_str());
	ImGui::Separator();

	if (AbstractSceneFactory* factory = sceneManager.GetSceneFactory()) {
		const std::vector<std::string> names = factory->GetSceneNames();
		if (names.empty()) {
			ImGui::TextDisabled("(SceneFactory::GetSceneNames が空です)");
		}
		for (const std::string& name : names) {
			if (ImGui::MenuItem(name.c_str(), nullptr, name == current, canChange)) {
				sceneManager.ChangeScene(name);
			}
		}
	}

	ImGui::Separator();
	if (ImGui::MenuItem("シーンをリロード", "Ctrl+R", false, canChange && !current.empty())) {
		sceneManager.ReloadCurrentScene();
	}
}

void DrawHelpMenu()
{
	ImGui::TextDisabled("ショートカット");
	ImGui::Separator();
	ImGui::Text("Ctrl+P    ウィンドウを検索して開く");
	ImGui::Text("Ctrl+S    全パラメータを保存");
	ImGui::Text("Ctrl+R    シーンをリロード");
	ImGui::Text("F5        再生 / 一時停止");
	ImGui::Text("F10       コマ送り（一時停止中）");
	ImGui::Separator();
	ImGui::TextDisabled("ギズモ");
	ImGui::Text("Ctrl+1/2/3  移動 / 回転 / 拡縮");
	ImGui::Text("Ctrl+L    ワールド ⇔ ローカル");
	ImGui::Text("Ctrl+G    ギズモの表示切替");
}

// --- 再生コントロール ---

void DrawPlayControls()
{
	const bool paused = DeltaTime::IsPaused();

	// 一時停止中はボタンを目立たせる
	if (paused) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.15f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.55f, 0.2f, 1.0f));
	}
	if (ImGui::Button(paused ? " 再生 " : " 一時停止 ")) {
		DeltaTime::SetPaused(!paused);
	}
	if (paused) {
		ImGui::PopStyleColor(2);
	}

	ImGui::BeginDisabled(!paused);
	if (ImGui::Button(" コマ送り ")) {
		DeltaTime::RequestStep(1);
	}
	ImGui::EndDisabled();

	float scale = DeltaTime::GetDebugTimeScale();
	ImGui::SetNextItemWidth(110.0f);
	if (ImGui::SliderFloat("##timescale", &scale, 0.05f, 2.0f, "x%.2f")) {
		DeltaTime::SetDebugTimeScale(scale);
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("ゲーム全体の再生速度。右クリックで数値入力");
	}
	if (ImGui::Button("x1")) {
		DeltaTime::SetDebugTimeScale(1.0f);
	}
}

// --- ショートカット ---

void HandleShortcuts()
{
	EditorGizmo::HandleShortcuts();

	if (ImGui::Shortcut(ImGuiKey_F5, ImGuiInputFlags_RouteGlobal)) {
		DeltaTime::SetPaused(!DeltaTime::IsPaused());
	}
	if (ImGui::Shortcut(ImGuiKey_F10, ImGuiInputFlags_RouteGlobal)) {
		// 止まっていなければ、まず止めてから1コマ進める
		if (!DeltaTime::IsPaused()) {
			DeltaTime::SetPaused(true);
		}
		DeltaTime::RequestStep(1);
	}
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
		const size_t count = GlobalVariables::GetInstance().SaveAllFiles();
		EditorMenuBar::ShowToast("%zu グループを保存しました", count);
	}
	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_R, ImGuiInputFlags_RouteGlobal)) {
		SceneManager::GetInstance().ReloadCurrentScene();
	}
}

// --- 通知 ---

void DrawToast()
{
	if (g_toastTimer <= 0.0f) {
		return;
	}
	// ポーズ中でも消えてほしいので実時間で減らす
	g_toastTimer -= DeltaTime::GetUnscaledDeltaTime();

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 16.0f,
			viewport->WorkPos.y + viewport->WorkSize.y - 16.0f),
		ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.85f);

	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoInputs;

	if (ImGui::Begin("##EditorToast", nullptr, flags)) {
		ImGui::TextUnformatted(g_toastText.c_str());
	}
	ImGui::End();
}

} // namespace

void EditorMenuBar::ShowToast(const char* format, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	g_toastText = buffer;
	g_toastTimer = kToastDuration;
}

void EditorMenuBar::Draw()
{
	HandleShortcuts();

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			DrawFileMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene")) {
			DrawSceneMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window")) {
			EditorWindow::DrawWindowMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Debug Draw")) {
			EditorDebugDraw::DrawMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Gizmo")) {
			EditorGizmo::DrawMenu();
			ImGui::SeparatorText("クリック選択");
			EditorPicking::DrawMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Layout")) {
			EditorLayout::DrawMenu();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help")) {
			DrawHelpMenu();
			ImGui::EndMenu();
		}

		// App 側が Editor::AddMenu() で足したメニュー。エンジンの標準メニューの右に並ぶ
		Editor::DrawExtraMenus();

		ImGui::Separator();
		DrawPlayControls();

		// 残り幅いっぱいを使って右寄せする
		EditorStats::DrawMenuBarSummary();

		ImGui::EndMainMenuBar();
	}

	EditorWindow::DrawQuickOpen();
	EditorStats::DrawWindow();
	DrawToast();
}

#endif // _DEBUG
