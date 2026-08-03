#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// 読み込み済みサウンドの一覧と試聴。Resource/sound の .wav も読み込める。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawAudioWindow();

} // namespace Editor

#endif // _DEBUG
