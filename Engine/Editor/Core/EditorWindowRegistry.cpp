#include "EditorWindowRegistry.h"
#ifdef _DEBUG

#include "Debugger/GlobalVariables.h"
#include <algorithm>
#include <cfloat>
#include <cstdint>
#include <map>
#include <utility>

namespace {

struct Entry {
	std::string category;
	EditorWindow::Origin origin = EditorWindow::Origin::Engine;
	bool visible = true;
	// 最後に Begin() が呼ばれたフレーム番号。
	// 「今のシーンに居ないウィンドウ」をメニュー上で灰色にするために持つ
	uint64_t lastSeenFrame = 0;
};

// 今どちらの drawer を回しているか。Editor::Draw() が drawer ごとに立てる
EditorWindow::Origin g_currentOrigin = EditorWindow::Origin::Engine;

// std::map はノードベースなので、要素を足しても既存要素のアドレスは動かない。
// GetVisibleFlag() が返す bool* を安全に配れるのはこの性質のおかげ
std::map<std::string, Entry> g_entries;

// 保存ファイルから読んだ表示状態。ウィンドウは「初めて描かれたとき」に遅延登録されるので、
// 登録の瞬間にここを引いて初期値を決める
std::map<std::string, bool> g_saved;

uint64_t g_frame = 0;

constexpr const char* kDirectoryName = "Editor";
constexpr const char* kGroupName = "EditorWindows";

Entry& Register(const char* name, const char* category, bool defaultVisible)
{
	auto it = g_entries.find(name);
	if (it != g_entries.end()) {
		// 名前が実行時に決まるウィンドウ（PostEffectなど）でも分類が付くよう、毎回上書きしておく
		if (category && *category) {
			it->second.category = category;
		}
		it->second.origin = g_currentOrigin;
		return it->second;
	}

	Entry entry;
	entry.category = (category && *category) ? category : EditorWindow::Category::kMisc;
	entry.origin = g_currentOrigin;
	auto savedIt = g_saved.find(name);
	entry.visible = (savedIt != g_saved.end()) ? savedIt->second : defaultVisible;

	return g_entries.emplace(name, std::move(entry)).first->second;
}

// 直近2フレーム以内に描かれたか。メニューを描くのはフレーム先頭（＝まだ今フレームの
// ウィンドウが描かれていない時点）なので、1フレーム前まで許容する
bool IsAlive(const Entry& entry)
{
	return entry.lastSeenFrame + 1 >= g_frame;
}

// 大文字小文字を無視した部分一致
bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle)
{
	if (needle.empty()) {
		return true;
	}
	auto it = std::search(haystack.begin(), haystack.end(), needle.begin(), needle.end(),
		[](char a, char b) {
			return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
		});
	return it != haystack.end();
}

} // namespace

bool EditorWindow::Begin(const char* name, const char* category, ImGuiWindowFlags flags, bool defaultVisible)
{
	Entry& entry = Register(name, category, defaultVisible);
	entry.lastSeenFrame = g_frame;

	if (!entry.visible) {
		return false;
	}

	// 閉じるボタンで entry.visible が落ちる
	if (!ImGui::Begin(name, &entry.visible, flags)) {
		// 折りたたまれている場合も ImGui::End は必要
		ImGui::End();
		return false;
	}
	return true;
}

void EditorWindow::End()
{
	ImGui::End();
}

bool* EditorWindow::GetVisibleFlag(const char* name, const char* category, bool defaultVisible)
{
	Entry& entry = Register(name, category, defaultVisible);
	entry.lastSeenFrame = g_frame;
	return &entry.visible;
}

bool EditorWindow::IsVisible(const char* name)
{
	auto it = g_entries.find(name);
	return (it != g_entries.end()) ? it->second.visible : false;
}

void EditorWindow::SetVisible(const char* name, bool visible)
{
	Register(name, Category::kMisc, visible).visible = visible;
}

void EditorWindow::NewFrame()
{
	++g_frame;
}

void EditorWindow::SetCurrentOrigin(Origin origin)
{
	g_currentOrigin = origin;
}

namespace {

// 指定した出所のウィンドウだけをカテゴリ別サブメニューに並べる。
// 1つも無ければ何も描かず false を返す。
//
// エンジンとゲームで同じカテゴリ名（"Camera" など）が出るため、
// 呼び出し側で PushID してからでないと BeginMenu のIDが衝突して片方が消える
bool DrawCategoriesOf(EditorWindow::Origin origin)
{
	// map なのでカテゴリ名・ウィンドウ名とも自動で昇順になる
	std::map<std::string, std::vector<std::pair<const std::string*, Entry*>>> byCategory;
	for (auto& pair : g_entries) {
		if (pair.second.origin != origin) {
			continue;
		}
		byCategory[pair.second.category].emplace_back(&pair.first, &pair.second);
	}
	if (byCategory.empty()) {
		return false;
	}

	const ImVec4 dimmed = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);

	for (auto& [category, list] : byCategory) {
		if (!ImGui::BeginMenu(category.c_str())) {
			continue;
		}
		for (auto& [namePtr, entry] : list) {
			// 今のシーンに居ないウィンドウは灰色。ただしクリックはできるままにしておく
			// （BeginDisabled にすると二度と開けなくなるため）
			const bool alive = IsAlive(*entry);
			if (!alive) {
				ImGui::PushStyleColor(ImGuiCol_Text, dimmed);
			}
			ImGui::MenuItem(namePtr->c_str(), nullptr, &entry->visible);
			if (!alive) {
				ImGui::PopStyleColor();
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
					ImGui::SetTooltip("このウィンドウは今のシーンでは描画されていません");
				}
			}
		}
		ImGui::EndMenu();
	}
	return true;
}

} // namespace

void EditorWindow::DrawWindowMenu()
{
	if (ImGui::MenuItem("すべて表示")) {
		SetAllVisible(true);
	}
	if (ImGui::MenuItem("すべて閉じる")) {
		SetAllVisible(false);
	}
	ImGui::Separator();

	if (g_entries.empty()) {
		ImGui::TextDisabled("(まだウィンドウがありません)");
		return;
	}

	// エンジンのウィンドウとゲームのウィンドウを分けて出す。
	// カテゴリ（Camera など）は両方に出てくるので、見出しが無いとどちらのものか分からない
	ImGui::TextDisabled("エンジン");
	ImGui::PushID("engine");
	DrawCategoriesOf(Origin::Engine);
	ImGui::PopID();

	ImGui::Separator();
	ImGui::TextDisabled("ゲーム");
	ImGui::PushID("app");
	const bool hasApp = DrawCategoriesOf(Origin::App);
	ImGui::PopID();
	if (!hasApp) {
		ImGui::TextDisabled("  (登録されていません)");
	}

	ImGui::Separator();
	if (ImGui::MenuItem("表示状態を保存")) {
		SaveSettings();
	}
}

void EditorWindow::DrawQuickOpen()
{
	static char filter[64] = "";
	static bool justOpened = false;

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, ImGuiInputFlags_RouteGlobal)) {
		filter[0] = '\0';
		justOpened = true;
		ImGui::OpenPopup("##QuickOpen");
	}

	// 画面上部中央のコマンドパレット風に出す
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		ImVec2(viewport->GetCenter().x, viewport->WorkPos.y + 60.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(440.0f, 0.0f), ImGuiCond_Always);

	if (!ImGui::BeginPopup("##QuickOpen")) {
		return;
	}

	if (justOpened) {
		ImGui::SetKeyboardFocusHere();
		justOpened = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	ImGui::InputTextWithHint("##filter", "ウィンドウ名で検索（Enterで開く / Escで閉じる）", filter, sizeof(filter));

	const bool submit = ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter);
	const std::string needle = filter;

	ImGui::Separator();

	const char* firstMatch = nullptr;
	int shown = 0;
	for (auto& [name, entry] : g_entries) {
		if (!ContainsIgnoreCase(name, needle)) {
			continue;
		}
		if (!firstMatch) {
			firstMatch = name.c_str();
		}
		if (shown >= 12) {
			continue;
		}
		++shown;

		if (ImGui::Selectable(name.c_str())) {
			entry.visible = true;
			ImGui::SetWindowFocus(name.c_str());
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("  %s / %s",
			entry.origin == EditorWindow::Origin::Engine ? "エンジン" : "ゲーム",
			entry.category.c_str());
	}

	if (shown == 0) {
		ImGui::TextDisabled("該当なし");
	}

	// Enterは常に先頭の候補を開く
	if (submit && firstMatch) {
		SetVisible(firstMatch, true);
		ImGui::SetWindowFocus(firstMatch);
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void EditorWindow::SetAllVisible(bool visible)
{
	for (auto& pair : g_entries) {
		pair.second.visible = visible;
	}
}

void EditorWindow::LoadSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);
	// ファイルが無ければ何も起きない（初回起動）
	gv.LoadFile(kDirectoryName, kGroupName);

	const GlobalVariables::json object = gv.ExportGroup(kGroupName);
	for (auto it = object.begin(); it != object.end(); ++it) {
		if (it->is_boolean()) {
			g_saved[it.key()] = it->get<bool>();
		}
	}
}

void EditorWindow::SaveSettings()
{
	GlobalVariables& gv = GlobalVariables::GetInstance();
	gv.CreateGroup(kGroupName);

	for (const auto& [name, entry] : g_entries) {
		gv.SetValue(kGroupName, name, entry.visible);
	}
	// 今回のシーンには出てこなかったウィンドウの設定も消さずに引き継ぐ
	for (const auto& [name, visible] : g_saved) {
		if (!g_entries.contains(name)) {
			gv.SetValue(kGroupName, name, visible);
		}
	}

	gv.SaveFile(kDirectoryName, kGroupName);
}

std::vector<std::string> EditorWindow::GetAllWindowNames()
{
	std::vector<std::string> names;
	names.reserve(g_entries.size());
	for (const auto& pair : g_entries) {
		names.push_back(pair.first);
	}
	return names;
}

#endif // _DEBUG
