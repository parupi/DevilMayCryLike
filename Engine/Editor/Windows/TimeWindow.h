#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// DeltaTime と TimeManager の状態表示。
/// もとは両クラスの Update() の中で ImGui を呼んでいたものをこちらへ移した。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawTimeWindows();

} // namespace Editor

#endif // _DEBUG
