#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// CameraManager が持つカメラの一覧・アクティブ切替と、選択中カメラのパラメータ表示。
/// Editor::Initialize で登録済み。
/// </summary>
void DrawCameraWindow();

} // namespace Editor

#endif // _DEBUG
