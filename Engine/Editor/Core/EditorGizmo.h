#pragma once
#ifdef _DEBUG

#include <imgui/imgui.h>

/// <summary>
/// ゲームビューの上に重ねる、選択オブジェクト操作用のギズモ。
///
/// Hierarchy / Inspector と同じ「選択」（EditorSelection）を対象にして、
/// Game ウィンドウに描いた絵の上でマウス操作させる。ImGuizmo のような外部ライブラリは
/// 使わず、エンジンの Matrix4x4 / Quaternion（行ベクトル・左手系）に合わせて自前で持っている。
///
/// 呼び出しは EditorGameView::DrawWindow() の ImGui::Image 直後の1箇所だけ:
///
/// <code>
/// ImGui::Image(GetTextureID(), imageSize);
/// EditorGizmo::DrawOverlay(ImGui::GetItemRectMin(), imageSize);
/// </code>
///
/// 編集するのは WorldTransform のローカル値（translation_ / rotation_ / scale_）。
/// 親がいる場合はワールドでの操作量を親のローカル空間へ落としてから書き戻すので、
/// 子オブジェクトでも見たとおりに動く。
/// </summary>
namespace EditorGizmo {

/// <summary>ギズモの操作モード</summary>
enum class Operation {
	Translate, // 移動
	Rotate,    // 回転
	Scale,     // 拡縮（常にローカル軸）
};

/// <summary>移動・回転の基準にする座標系</summary>
enum class Space {
	World, // ワールド軸
	Local, // オブジェクトの姿勢に沿った軸
};

// --- 状態 ---

/// <summary>ギズモを表示・操作できるか</summary>
bool IsEnabled();
void SetEnabled(bool enabled);

Operation GetOperation();
void SetOperation(Operation operation);

Space GetSpace();
void SetSpace(Space space);

/// <summary>今ドラッグ中か（ゲーム側の入力を止めたいときに使う）</summary>
bool IsUsing();

/// <summary>ハンドルにカーソルが乗っているか。ドラッグ中も true</summary>
bool IsOver();

// --- 駆動 ---

/// <summary>
/// ゲームビューの画像の上にギズモを描いて操作させる。
/// ImGui::Image を出した直後、同じウィンドウの中から呼ぶこと。
/// </summary>
/// <param name="imagePos">画像の左上のスクリーン座標（ImGui::GetItemRectMin()）</param>
/// <param name="imageSize">画像の表示サイズ</param>
void DrawOverlay(const ImVec2& imagePos, const ImVec2& imageSize);

/// <summary>"Gizmo" メニューの中身を描く（BeginMenu/EndMenu の内側で呼ぶ）</summary>
void DrawMenu();

/// <summary>ショートカットを処理する。毎フレーム1回だけ呼ぶ</summary>
void HandleShortcuts();

/// <summary>保存された設定を読み込む。起動時に一度だけ</summary>
void LoadSettings();
/// <summary>現在の設定を保存する</summary>
void SaveSettings();

} // namespace EditorGizmo

#endif // _DEBUG
