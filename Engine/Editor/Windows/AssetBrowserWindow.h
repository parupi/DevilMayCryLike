#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// 読み込み済みのモデル／テクスチャの一覧と、ディスク上のモデルの読み込み。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawAssetBrowserWindow();

} // namespace Editor

#endif // _DEBUG
