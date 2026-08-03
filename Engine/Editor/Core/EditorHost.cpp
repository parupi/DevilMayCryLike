#include "EditorHost.h"
#ifdef _DEBUG

#include "EditorDebugDraw.h"
#include "EditorMenuBar.h"
#include "EditorStats.h"
#include "Editor/Windows/AssetBrowserWindow.h"
#include "Editor/Windows/AudioWindow.h"
#include "Editor/Windows/CameraWindow.h"
#include "Editor/Windows/DebugLogWindow.h"
#include "Editor/Windows/HierarchyWindow.h"
#include "Editor/Windows/InspectorWindow.h"
#include "Editor/Windows/LightWindow.h"
#include "Editor/Windows/ProfilerWindow.h"
#include "Editor/Windows/ParticleEditorWindow.h"
#include "Editor/Windows/RenderWindow.h"
#include "Editor/Windows/TimeWindow.h"

#include <string>
#include <utility>
#include <vector>

namespace {

struct MenuEntry {
	std::string label;
	std::vector<Editor::DrawFunc> bodies;
};

struct DrawerEntry {
	Editor::DrawFunc draw;
	EditorWindow::Origin origin;
};

std::vector<DrawerEntry> g_drawers;
std::vector<MenuEntry> g_menus;

// Editor::Initialize() の実行中だけ true。ここで登録された drawer がエンジン標準になる
bool g_registeringEngineDefaults = false;

} // namespace

void Editor::AddWindowDrawer(DrawFunc drawer, EditorWindow::Origin origin)
{
	if (!drawer) {
		return;
	}
	// Editor::Initialize() の中から登録されたものは、引数によらずエンジン標準として扱う
	if (g_registeringEngineDefaults) {
		origin = EditorWindow::Origin::Engine;
	}
	g_drawers.push_back({ std::move(drawer), origin });
}

void Editor::AddMenu(const char* label, DrawFunc drawMenuBody)
{
	if (!label || !*label || !drawMenuBody) {
		return;
	}
	for (MenuEntry& menu : g_menus) {
		if (menu.label == label) {
			menu.bodies.push_back(std::move(drawMenuBody));
			return;
		}
	}
	MenuEntry entry;
	entry.label = label;
	entry.bodies.push_back(std::move(drawMenuBody));
	g_menus.push_back(std::move(entry));
}

void Editor::Initialize()
{
	// ウィンドウの表示状態とデバッグ描画の設定を前回終了時の状態に戻す。
	// ウィンドウ自体は初めて描かれたときに遅延登録されるので、ここでは値だけ用意しておけばよい
	EditorWindow::LoadSettings();
	EditorDebugDraw::LoadSettings();

	EditorLayout::RegisterBuiltinPresets();

	// エンジン標準のウィンドウ。App のウィンドウはこの後ろに並ぶ。
	// このスコープで登録したものが Windowメニューの「エンジン」側になる。
	// Hierarchy → Inspector の順に呼ぶ（同じフレームの選択変更がすぐ反映される）
	g_registeringEngineDefaults = true;
	AddWindowDrawer([] { DrawHierarchyWindow(); });
	AddWindowDrawer([] { DrawInspectorWindow(); });
	AddWindowDrawer([] { DrawAssetBrowserWindow(); });
	AddWindowDrawer([] { DrawLightWindow(); });
	AddWindowDrawer([] { DrawRenderWindow(); });
	AddWindowDrawer([] { DrawCameraWindow(); });
	AddWindowDrawer([] { DrawParticleEditorWindows(); });
	AddWindowDrawer([] { DrawAudioWindow(); });
	AddWindowDrawer([] { DrawTimeWindows(); });
	AddWindowDrawer([] { DrawProfilerWindow(); });
	AddWindowDrawer([] { DrawDebugLogWindow(); });
	g_registeringEngineDefaults = false;
}

void Editor::Finalize()
{
	// 次回起動時に同じ配置で開けるよう、終了時に必ず書き出しておく
	EditorWindow::SaveSettings();
	EditorDebugDraw::SaveSettings();
	EditorStats::Finalize();

	g_drawers.clear();
	g_menus.clear();
}

void Editor::DrawExtraMenus()
{
	for (MenuEntry& menu : g_menus) {
		if (!ImGui::BeginMenu(menu.label.c_str())) {
			continue;
		}
		for (DrawFunc& body : menu.bodies) {
			body();
		}
		ImGui::EndMenu();
	}
}

void Editor::Draw()
{
	// このフレームに描かれたウィンドウの記録をリセットする（メニューの表示に使う）
	EditorWindow::NewFrame();
	EditorStats::Update();

	// メインビューポート全体をドッキング先にする。
	// 素通し(PassthruCentralNode)にはしない。ゲームの絵はGameウィンドウの中にあるので、
	// 背景はエディタらしく塗り潰しておいたほうが見やすい
	const ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
	// Layoutメニューからプリセットが予約されていれば、ここで組み直す。
	// DockBuilderはDockSpaceを作った直後でないと正しく分割できない
	EditorLayout::ApplyPendingPreset(dockspaceId);

	// 左上のメニューバー。各エディタの表示切替はここから
	EditorMenuBar::Draw();

	// 描画中に AddWindowDrawer される可能性があるので、範囲for ではなく添字で回す。
	// その回に足された分は次のフレームから描かれる
	const size_t count = g_drawers.size();
	for (size_t i = 0; i < count; ++i) {
		// この drawer が開くウィンドウの出所を宣言してから呼ぶ。
		// レジストリが登録の瞬間に引くので、Begin() の呼び出し側は何も意識しなくてよい
		EditorWindow::SetCurrentOrigin(g_drawers[i].origin);
		g_drawers[i].draw();
	}
	EditorWindow::SetCurrentOrigin(EditorWindow::Origin::Engine);
}

#endif // _DEBUG
