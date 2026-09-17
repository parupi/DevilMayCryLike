#pragma once
#ifdef _DEBUG

#include <imgui/imgui.h>

/// <summary>
/// ゲームのカメラに割り込んで自由に飛び回るデバッグカメラ。
///
/// F9 で現在のカメラと切り替える。入れた瞬間の位置・向き・画角をそのまま引き継ぐので
/// 画が飛ばない。切っている間もゲーム側のカメラは動き続けているため、戻したときも同じ。
///
/// 操作はマインクラフトのクリエイティブ飛行と同じ感覚:
///   右ドラッグ  … 視点を回す（押している間だけが「飛行モード」）
///   W / A / S / D … 向いている方向へ前後・左右
///   Space / Shift … 上昇・下降（ワールドの上下）
///   Ctrl          … ダッシュ
///   ホイール      … 移動速度の増減
///
/// ゲームビューの中でマウスを掴んでいる間だけ飛行モードに入り、その間は
/// `Input::SetSuppressedForEditor()` でゲームへの入力を止める
/// （同じ WASD でプレイヤーまで走り出すため）。
///
/// カメラの実体はこのモジュールが持ち、`CameraManager::SetDebugCamera()` で
/// ポインタだけ渡す。`cameras_` に入れるとシーン切替の `DeleteAllCamera()` で消えてしまう。
/// </summary>
namespace EditorCamera {

/// <summary>デバッグカメラに切り替わっているか</summary>
bool IsActive();

/// <summary>デバッグカメラの ON / OFF。ON にした時点のカメラの位置と向きを引き継ぐ</summary>
void SetActive(bool active);

/// <summary>ON / OFF を反転する（F9）</summary>
void Toggle();

/// <summary>今マウスを掴んで飛行操作中か。この間ゲームへの入力は止まっている</summary>
bool IsFlying();

/// <summary>
/// 入力を読んでカメラを動かす。毎フレーム1回、`CameraManager::Update()` より前に呼ぶこと。
/// ゲームビューが隠れているフレームも呼ぶ（掴んだままの状態を解除するため）。
/// </summary>
/// <param name="gameViewHovered">ゲームビューの上にカーソルが乗っているか</param>
void Update(bool gameViewHovered);

/// <summary>ゲームビューの隅に状態を出す。ImGui::Image の後で呼ぶ</summary>
void DrawBadge(const ImVec2& imagePos, const ImVec2& imageSize);

/// <summary>Camera ウィンドウに出す設定UI</summary>
void DrawSettings();

/// <summary>割り込みを外してカメラを捨てる</summary>
void Finalize();

void LoadSettings();
void SaveSettings();

} // namespace EditorCamera

#endif // _DEBUG
