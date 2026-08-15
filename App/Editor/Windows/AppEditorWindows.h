#pragma once
#ifdef _DEBUG

/// <summary>
/// ゲーム固有のエディタウィンドウ。すべて AppEditor::Register() から登録される。
/// 対象は AppEditor::FindPlayer() / FindGameCamera() などで毎フレーム引き直すので、
/// 対象がいないシーンでは各ウィンドウがその旨を出すだけで済む。
/// </summary>
namespace AppEditor {

void DrawPlayerWindow();        // プレイヤーの状態 + 攻撃プレビュー
void DrawAttackEditorWindows(); // Attack Editor / Attack Derivative Editor
void DrawEnemyWindow();         // シーン内の敵一覧とHP
void DrawCameraWorkWindow();    // GameCamera のカメラワーク調整
void DrawStylishWindow();       // スタイリッシュランク
void DrawHitEffectWindows();    // HitEffect / HitPostEffect
void DrawTrainingWindow();      // トレーニングルームの操作と起動時の行き先

} // namespace AppEditor

#endif // _DEBUG
