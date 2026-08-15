#pragma once
#include "Scene/BaseScene.h"

#include "GameObject/Camera/GameCamera.h"
#include "World3D/Light/LightManager.h"
#include "GameObject/UI/StageStart/StageStart.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Camera/ClearCamera.h"
#include "GameObject/UI/GameUI/GameUI.h"
#include "Scene/GameScene/State/GameSceneStateBase.h"
#include "GameObject/UI/Menu/MenuUI.h"
#include "GameObject/UI/Menu/GameOverUI.h"
#include "GameObject/UI/StyleHUD/StyleHUD.h"
#include "GameObject/Training/TrainingController.h"
#include "GameObject/UI/Training/TrainingHUD.h"
#include <memory>
#include "Input/InputContext.h"
#include "Tutorial/System/TutorialSystem.h"
#include "Graphics/Rendering/Sprite/AnimatedSprite.h"

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

#ifdef _DEBUG
	/// <summary>
	/// デバッグ用更新処理  
	/// ImGuiを用いたデバッグ描画やパラメータのリアルタイム調整を行う。
	/// </summary>
	void DebugUpdate() override;
#endif // _DEBUG

	/// <summary>トレーニングルームとして動いているか（本編ならfalse）</summary>
	bool IsTrainingMode() const;

	/// <summary>
	/// トレーニングの操作。本編では nullptr。
	/// エディタの Training ウィンドウもここから引く
	/// </summary>
	TrainingController* GetTrainingController() { return training_.get(); }

	// ステートを切り替える
	void ChangeState(const std::string& stateName);

	void SetSceneTime(float time) { sceneDeltaTime_ = time; }

	float GetMaskAlpha() const { return maskAlpha_; }
	void SetMaskAlpha(float alpha) { maskAlpha_ = alpha; }

	// メニューのUIをまとめたクラスを取得
	MenuUI* GetMenuUI() { return menuUI_.get(); }
	GameOverUI* GetGameOverUI() { return gameOverUI_.get(); }
	// 入力の受付状態を管理するクラスを取得
	InputContext* GetInputContext() { return inputContext_.get(); }

	TutorialService* GetTutorialService() { return tutorial_.get(); }
	// プレイヤーを取得（ステート側からスコア・戦闘状態を引くのに使う）
	Player* GetPlayer() { return player_; }
private:
	std::unordered_map<std::string, std::unique_ptr<GameSceneStateBase>> states_;
	GameSceneStateBase* currentState_ = nullptr;
	// 入力をまとめたクラス
	std::unique_ptr<InputContext> inputContext_ = nullptr;
	// ロックオンの処理を行うクラス
	std::unique_ptr<LockOnSystem> lockOnSystem_ = nullptr;

	CameraManager* cameraManager_ = &CameraManager::GetInstance(); ///< カメラ管理クラス
	GameCamera* gameCamera_ = nullptr; ///< ゲームシーン専用カメラ

	LightManager* lightManager_ = &LightManager::GetInstance(); ///< ライト管理クラス

	Player* player_ = nullptr; ///< プレイヤーオブジェクトへのポインタ

	std::unique_ptr<GameUI> gameUI_;

	// マスクを掛けるためのスプライト
	Sprite* mask_ = nullptr;
	float maskAlpha_ = 0.0f;

	// メニューのスプライト
	std::unique_ptr<MenuUI> menuUI_ = nullptr;
	std::unique_ptr<GameOverUI> gameOverUI_ = nullptr;
	// スタイリッシュランクのゲーム中HUD
	std::unique_ptr<StyleHUD> styleHud_ = nullptr;

	// トレーニングルーム。本編では作らない
	std::unique_ptr<TrainingController> training_ = nullptr;
	std::unique_ptr<TrainingHUD> trainingHud_ = nullptr;
	// シーン全体のデルタタイム
	float sceneDeltaTime_ = 0.0f;

	std::unique_ptr<TutorialSystem> tutorial_ = nullptr;
};
