#pragma once
#ifdef _DEBUG

// エディタを外から拡張するときは、このヘッダ1枚を include すれば足りる
#include "EditorContext.h"
#include "EditorLayout.h"
#include "EditorWindowRegistry.h"

#include <functional>

/// <summary>
/// エディタの駆動と拡張ポイント。
///
/// エンジンのエディタは App のことを知らない。代わりにここへ差し込んでもらう:
///
/// <code>
/// // App/Editor/AppEditor.cpp
/// void AppEditor::Register() {
///     Editor::AddWindowDrawer([] { DrawPlayerWindow(); });
///     Editor::AddMenu("Game", [] { ImGui::MenuItem("敵を沸かせる"); });
///     Editor::AddLayoutPreset({ "バトル調整", { "Game" }, ... });
/// }
/// </code>
///
/// 呼ぶ場所は MyGameTitle::Initialize()。App と Engine の両方を知っている
/// 唯一の合流点なので、依存の向き（App/Editor → Engine/Editor → Engine）を壊さずに済む。
/// </summary>
namespace Editor {

using DrawFunc = std::function<void()>;

// --- 拡張ポイント ---

/// <summary>
/// 毎フレーム呼ばれる描画関数を足す。中で EditorWindow::Begin/End を使うこと。
/// 登録した順に呼ばれる。
///
/// この drawer から開かれたウィンドウは Windowメニューの「ゲーム」側に並ぶ。
/// エンジン標準のウィンドウ（Editor::Initialize が登録するもの）と区別するための仕分けで、
/// App から呼ぶぶんには何も意識しなくてよい。
///
/// エンジン側の都合で Editor::Initialize の外から登録するものだけ、
/// 明示的に Origin::Engine を渡す（ImGuiManager が持つゲームビューなど）。
/// </summary>
void AddWindowDrawer(DrawFunc drawer, EditorWindow::Origin origin = EditorWindow::Origin::App);

/// <summary>
/// メニューバーに独自メニューを足す。drawMenuBody は BeginMenu/EndMenu の内側で呼ばれる。
/// 同じ label で二度呼ぶと、後から足したほうが同じメニューの続きに並ぶ。
/// </summary>
void AddMenu(const char* label, DrawFunc drawMenuBody);

// AddLayoutPreset は EditorLayout.h で宣言している

// --- 駆動（ImGuiManager から呼ぶ。ゲーム側が触る必要はない） ---

/// <summary>保存済み設定の読み込みと、エンジン標準ウィンドウ／プリセットの登録</summary>
void Initialize();

/// <summary>設定の保存と後始末</summary>
void Finalize();

/// <summary>
/// エディタ一式を描く。ImGui::NewFrame() の直後に1回だけ呼ぶ。
/// DockSpace の構築・メニューバー・登録された全ウィンドウがこの中で走る。
/// </summary>
void Draw();

/// <summary>AddMenu で足されたメニューを描く。EditorMenuBar の内部用</summary>
void DrawExtraMenus();

} // namespace Editor

#endif // _DEBUG
