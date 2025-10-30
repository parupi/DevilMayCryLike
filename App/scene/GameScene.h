#pragma once
#include <scene/BaseScene.h>
#include <memory>
#include <GameObject/Camera/GameCamera.h>
#include <3d/Light/LightManager.h>
#include <GameObject/UI/StageStart/StageStart.h>
#include <GameObject/Player/Player.h>

/// <summary>
/// ゲーム本編のシーンを管理するクラス  
/// 
/// プレイヤー・敵・カメラ・ライト・UIなど、  
/// ゲーム中に動作する主要なオブジェクトの生成・更新・描画処理を統括する。
/// </summary>
class GameScene : public BaseScene
{
public:
	GameScene() = default;
	~GameScene() = default;

	/// <summary>
	/// シーンの初期化処理  
	/// プレイヤー、カメラ、ライト、ステージなど、  
	/// ゲームプレイに必要な全てのオブジェクトを生成・初期化する。
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// シーンの終了処理  
	/// 使用中のリソースや動的に生成したオブジェクトを解放する。
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// シーンの更新処理  
	/// プレイヤーや敵、カメラなどのゲーム進行ロジックを更新する。
	/// </summary>
	void Update() override;

	/// <summary>
	/// シーンの描画処理  
	/// 3Dモデル、パーティクル、スプライト、UIなどの描画を行う。
	/// </summary>
	void Draw() override;

	/// <summary>
	/// RTVへの描画処理  
	/// Render Target Viewに対してシーンの描画を実行する。
	/// </summary>
	void DrawRTV() override;

#ifdef _DEBUG
	/// <summary>
	/// デバッグ用更新処理  
	/// ImGuiを用いたデバッグ描画やパラメータのリアルタイム調整を行う。
	/// </summary>
	void DebugUpdate() override;
#endif // _DEBUG

private:
	CameraManager* cameraManager_ = CameraManager::GetInstance(); ///< カメラ管理クラス
	GameCamera* gameCamera_ = nullptr; ///< ゲームシーン専用カメラ

	LightManager* lightManager_ = LightManager::GetInstance(); ///< ライト管理クラス

	Player* player_ = nullptr; ///< プレイヤーオブジェクトへのポインタ

	std::unique_ptr<Sprite> deathUI_ = nullptr;

	StageStart stageStart_; ///< ステージ開始時のカメラ演出（「STAGE START」など）
};
