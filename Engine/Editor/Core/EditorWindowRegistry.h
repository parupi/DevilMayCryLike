#pragma once
#ifdef _DEBUG

#include <imgui/imgui.h>
#include <string>
#include <vector>

/// <summary>
/// エディタウィンドウの一元管理。
///
/// ウィンドウは「描かれたときに勝手に登録される」。一覧を別に持たないので追加漏れが起きない。
/// 使い方は ImGui::Begin/End をそのまま置き換えるだけ:
///
/// <code>
/// if (EditorWindow::Begin("GameCamera", EditorWindow::Category::kCamera)) {
///     // 中身
///     EditorWindow::End();
/// }
/// </code>
///
/// Begin() は「非表示にされている」「折りたたまれている」のどちらでも false を返す。
/// false のときは End() を呼ばないこと（ImGui::End の対応は Begin 側で済ませている）。
///
/// 表示状態は Resource/GlobalVariables/Editor/EditorWindows.json に保存され、
/// 次回起動時に復元される。
/// </summary>
namespace EditorWindow {

/// <summary>
/// Windowメニューのサブメニュー名。文字列なのでここに無い名前も自由に使える。
/// </summary>
namespace Category {
	inline constexpr const char* kGame = "Game";
	inline constexpr const char* kCharacter = "Character";
	inline constexpr const char* kWorld = "World";
	inline constexpr const char* kCamera = "Camera";
	inline constexpr const char* kVFX = "VFX / Particle";
	inline constexpr const char* kLighting = "Lighting";
	inline constexpr const char* kPostEffect = "PostEffect";
	inline constexpr const char* kScore = "Score";
	inline constexpr const char* kEngine = "Engine";
	inline constexpr const char* kMisc = "Misc";
} // namespace Category

/// <summary>
/// そのウィンドウがエンジン由来かゲーム由来か。Windowメニューの区切りに使う。
///
/// カテゴリ（Camera / VFX など）はエンジンとゲームで共有したいので、分類とは別軸で持つ。
/// 判定は自動で、Begin() の呼び出し側は何も意識しなくてよい:
/// Editor::Draw() が drawer を呼ぶ前に「今どちらの drawer を回しているか」を
/// SetCurrentOrigin() で立てておき、登録の瞬間にそれを引く。
/// </summary>
enum class Origin {
	Engine, // Engine/Editor/Windows/ 由来
	App,    // App/Editor/Windows/ 由来
};

/// <summary>これから呼ぶ drawer の出所を宣言する。EditorHost が使う</summary>
void SetCurrentOrigin(Origin origin);

/// <summary>
/// ウィンドウを開始する。未登録なら初回呼び出し時に登録される。
/// </summary>
/// <param name="name">ImGuiのウィンドウ名。レジストリのキーにもなる</param>
/// <param name="category">Windowメニューでの分類</param>
/// <param name="flags">ImGui::Begin にそのまま渡すフラグ</param>
/// <param name="defaultVisible">初回起動時（保存が無いとき）に開いておくか</param>
/// <returns>中身を描いてよければ true。true のときだけ End() を呼ぶこと</returns>
bool Begin(const char* name, const char* category, ImGuiWindowFlags flags = 0, bool defaultVisible = true);

/// <summary>Begin() が true を返したときだけ呼ぶ</summary>
void End();

/// <summary>
/// ImGui::Begin を自前で呼びたいウィンドウ用に、表示フラグだけを登録／取得する。
/// 戻り値のポインタはレジストリが生きている限り有効。
/// </summary>
bool* GetVisibleFlag(const char* name, const char* category, bool defaultVisible = true);

/// <summary>そのウィンドウが表示状態か（未登録なら defaultVisible 扱い）</summary>
bool IsVisible(const char* name);
/// <summary>表示状態を設定する（未登録なら登録される）</summary>
void SetVisible(const char* name, bool visible);

/// <summary>ImGuiのフレーム先頭で呼ぶ。「今フレーム描かれたか」の記録をリセットする</summary>
void NewFrame();

/// <summary>"Window" メニューの中身を描く（BeginMenu/EndMenu の内側で呼ぶ）</summary>
void DrawWindowMenu();

/// <summary>Ctrl+P のクイックオープン。毎フレーム呼ぶ（内部でショートカットを見ている）</summary>
void DrawQuickOpen();

/// <summary>登録済みの全ウィンドウの表示／非表示をまとめて切り替える</summary>
void SetAllVisible(bool visible);

/// <summary>保存された表示状態を読み込む。ImGuiの初期化直後に一度だけ呼ぶ</summary>
void LoadSettings();
/// <summary>現在の表示状態を保存する</summary>
void SaveSettings();

/// <summary>登録済みウィンドウ名の一覧（レイアウトプリセットなどが使う）</summary>
std::vector<std::string> GetAllWindowNames();

} // namespace EditorWindow

#endif // _DEBUG
