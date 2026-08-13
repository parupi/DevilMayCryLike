#pragma once
#ifdef _DEBUG

namespace Editor {

/// <summary>
/// シーンにいるスキンモデル1体を選んで、その再生状態を触るウィンドウ。
///
/// 見られるもの／触れるもの:
///   - クリップ一覧と再生（ブレンド時間・頭出しのやり直し・速度・ループ）
///   - 再生位置のスクラブ
///   - 上半身レイヤー（クリップとマスク基点ジョイント）
///   - ルートモーションの抽出ジョイントと、実際に出ている移動量
///   - アニメーションイベントの追加／削除／時刻編集と .anim.json への保存
///   - ジョイント一覧（ボーン追従させたいときの名前探し用）
///
/// Editor::Initialize で登録済み。
/// </summary>
void DrawAnimationWindow();

} // namespace Editor

#endif // _DEBUG
