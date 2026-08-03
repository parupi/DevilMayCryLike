#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// 各マネージャが今いくつ抱えているかの一覧。
/// フレームタイムやVRAMは Stats ウィンドウ（EditorStats）が担当する。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawProfilerWindow();

} // namespace Editor

#endif // _DEBUG
