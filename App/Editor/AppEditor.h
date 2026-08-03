#pragma once
#ifdef _DEBUG

class Player;
class GameCamera;

/// <summary>
/// アプリ側のエディタ。
///
/// エンジンのエディタ（Engine/Editor/）は App のことを何も知らない。
/// ゲーム固有のウィンドウ・メニュー・レイアウトはすべてここから
/// Editor::AddWindowDrawer / AddMenu / AddLayoutPreset で差し込む。
///
/// 依存の向きは App/Editor → Engine/Editor → Engine ランタイム。逆流させないこと。
///
/// 注意: include ディレクトリに Engine と App の両方が入っているので、
/// このフォルダにEngine/Editor/配下と同名のファイルを置かないこと（先に Engine 側が引かれる）。
/// </summary>
namespace AppEditor {

/// <summary>
/// エディタへの登録をまとめて行う。MyGameTitle::Initialize() から一度だけ呼ぶ。
/// Editor::SetContext() より後であること。
/// </summary>
void Register();

// --- 編集対象の取得 ---
// シーンから参照を渡してもらう代わりに、エンジンのマネージャから毎フレーム引き直す。
// シーンを跨いでも壊れず、対象が居ないシーンでは素直に nullptr が返る。
// GameScene 側に手を入れる必要もない。

/// <summary>現在のシーンにいる Player。いなければ nullptr</summary>
Player* FindPlayer();

/// <summary>CameraManager に登録されている GameCamera。いなければ nullptr</summary>
GameCamera* FindGameCamera();

} // namespace AppEditor

#endif // _DEBUG
