#pragma once
#include <string>

/// <summary>
/// 「今回のプレイをどう始めるか」。タイトルが決めて GameScene が読む。
///
/// シーンは切り替わるたびに作り直されるので、シーンのメンバでは渡せない。
/// StageDocument と同じく、翻訳単位ローカルの静的変数を関数越しに触る形にしてある。
/// 保存はしない（ファイルへ残るのは GameSettings の方だけ）。
/// </summary>
enum class GameMode {
	Story,    ///< 本編。ステージは StageDocument が指すものを読む
	Training, ///< トレーニングルーム。専用ステージで敵1体と1vs1する
};

namespace GameSession {

/// <summary>トレーニングルームのステージ。エディタで開いている本編ステージとは無関係に固定</summary>
constexpr const char* kTrainingStagePath = "Resource/Stage/Training.json";

GameMode GetMode();

/// <summary>本編として始める。タイトルの GAME START が呼ぶ</summary>
void BeginStory();

/// <summary>
/// トレーニングとして始める。タイトルの TRAINING が呼ぶ。
///
/// クラス名は Object3dFactory の登録キー（EnemyCatalog が持っているもの）で、
/// 省略すると一覧の先頭から始まる。タイトルでは相手を聞かずに部屋へ入り、
/// 中の設定メニュー（TrainingMenu）で切り替えるので、通常はこちらを通る。
/// 名指しするのは Debug の起動時トレーニングだけ
/// </summary>
void BeginTraining(const std::string& enemyClassName = std::string());

/// <summary>トレーニングで戦う敵のクラス名。本編では空</summary>
const std::string& GetTrainingEnemyClass();

/// <summary>今のモードで読むべきステージのパス</summary>
std::string GetStagePath();

// ======================
// 起動時の行き先（Debug 限定）
// ======================

/// <summary>
/// 起動時に最初に入るシーン名を決める。MyGameTitle が最初の ChangeScene に使う。
///
/// 通常は "TITLE"。Debug ビルドで「起動時トレーニング」が有効なら、
/// トレーニングとして始める準備をしてから "GAMEPLAY" を返す。
/// 敵の調整のたびにタイトル→メニュー→ステージ読み込みを通る手間を省くためのもので、
/// Release では常に "TITLE"（テスト用の入り口が製品に混ざらないように）
/// </summary>
std::string ResolveBootScene();

#ifdef _DEBUG

/// <summary>Debug 限定の起動設定。Resource/GlobalVariables/Editor/Training.json に残る</summary>
struct BootSettings {
	bool bootToTraining = false;
	/// <summary>直行するときに出す敵。空なら EnemyCatalog の先頭</summary>
	std::string enemyClassName;
};

const BootSettings& GetBootSettings();

/// <summary>設定を書き換えてファイルへ保存する。次の起動から効く</summary>
void SetBootSettings(const BootSettings& settings);

#endif // _DEBUG

} // namespace GameSession
