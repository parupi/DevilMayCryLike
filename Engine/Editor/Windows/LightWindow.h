#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// LightManager が持つライトの一覧・追加・削除・個別編集。
/// もとは LightManager::DrawLightEditor() にあったものをこちらへ移した。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawLightWindow();

} // namespace Editor

#endif // _DEBUG
