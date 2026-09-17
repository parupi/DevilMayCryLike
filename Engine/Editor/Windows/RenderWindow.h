#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// ポストエフェクトのチェーン（適用順・ON/OFF）と、スカイボックス／シャドウの設定。
/// CSM は自前のウィンドウを持っているので、ここから続けて呼ぶ。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawRenderWindow();

} // namespace Editor

#endif // _DEBUG
