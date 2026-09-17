#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// SE を合成して作るエディタ（設計書 Phase 7）。
///
/// 左で .sound を選び、右でレイヤーとエフェクトを触ると、その場で焼き直して試聴できる。
/// 保存すると Resource/Sounds/&lt;名前&gt;.sound になり、
/// ゲームからは <c>SoundManager::PlaySE("名前")</c> で鳴らせる。
///
/// Editor::Initialize で登録済み。
/// </summary>
void DrawSoundEditorWindow();

} // namespace Editor

#endif // _DEBUG
