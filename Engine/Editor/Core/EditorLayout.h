#pragma once
#ifdef _DEBUG

#include <imgui/imgui.h>
#include <string>
#include <vector>

namespace Editor {

/// <summary>
/// ドッキングレイアウトのプリセット1つ分。
///
/// 作業内容ごとに「開くウィンドウの組」がだいたい決まっているので、
/// 手で並べ直さなくても切り替えられるようにする。
/// プリセットを適用すると、そこに載っているウィンドウは自動的に表示状態になる。
///
/// ウィンドウ名は EditorWindow::Begin に渡している名前と完全に一致させること。
/// ここに無いウィンドウはプリセット適用後もフロート表示のまま残る。
/// center 以外は空でよく、空の領域は分割そのものが行われない。
/// </summary>
struct LayoutPreset {
	std::string name;
	std::vector<std::string> center;
	std::vector<std::string> left;
	std::vector<std::string> right;
	std::vector<std::string> bottom;
	float leftRatio = 0.20f;
	float rightRatio = 0.22f;
	float bottomRatio = 0.26f;
};

/// <summary>
/// レイアウトプリセットを追加する。Layoutメニューに並ぶ。
/// アプリ側のプリセットはここから足すこと（エンジンはアプリのウィンドウ名を知らない）。
/// </summary>
void AddLayoutPreset(LayoutPreset preset);

} // namespace Editor

/// <summary>プリセットの適用まわり。エディタの駆動側（EditorHost / EditorMenuBar）から呼ぶ</summary>
namespace EditorLayout {

/// <summary>エンジン標準のプリセットを登録する。Editor::Initialize から一度だけ呼ぶ</summary>
void RegisterBuiltinPresets();

/// <summary>次のフレームでプリセットを適用するよう予約する</summary>
void RequestPreset(int index);

/// <summary>
/// 予約されたプリセットがあれば適用する。
/// DockSpace を作った直後（同じフレーム内）に呼ぶこと。
/// </summary>
void ApplyPendingPreset(ImGuiID dockspaceId);

/// <summary>"Layout" メニューの中身を描く（BeginMenu/EndMenu の内側で呼ぶ）</summary>
void DrawMenu();

} // namespace EditorLayout

#endif // _DEBUG
