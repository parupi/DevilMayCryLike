#pragma once
#ifdef _DEBUG

/// <summary>
/// FPS・VRAM・RAM の計測と表示。
///
/// VRAMは以前チュートリアルのGIFで1.6GB近くまで膨らんだことがあるので、
/// 数字が常にメニューバーに出ているだけで再発に気づける。
/// </summary>
namespace EditorStats {

/// <summary>毎フレーム先頭で呼ぶ。計測値の更新のみ（描画はしない）</summary>
void Update();

/// <summary>メニューバー右端に出す1行サマリ</summary>
void DrawMenuBarSummary();

/// <summary>詳細ウィンドウ（フレームタイムのグラフなど）</summary>
void DrawWindow();

/// <summary>DXGIアダプタなどの参照を解放する。ImGuiManager::Finalize から呼ぶ</summary>
void Finalize();

} // namespace EditorStats

#endif // _DEBUG
