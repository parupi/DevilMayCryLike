#pragma once
#ifdef _DEBUG

/// <summary>
/// 画面左上のメニューバー。エディタの入口をここに集約している。
///
/// File / Scene / Window / Debug Draw / Layout / Help のメニューに加えて、
/// 右側に再生コントロール（一時停止・コマ送り・速度）と FPS / VRAM を出す。
/// </summary>
namespace EditorMenuBar {

/// <summary>
/// メニューバーと、それに付随するウィンドウ（クイックオープン・Stats・通知）を描く。
/// ImGuiのフレーム内、DockSpaceを作った後に呼ぶこと。
/// </summary>
void Draw();

/// <summary>画面右下に数秒だけ出る通知。保存完了などの合図に使う</summary>
void ShowToast(const char* format, ...);

} // namespace EditorMenuBar

#endif // _DEBUG
