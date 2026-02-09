#pragma once
#include "scene/BaseScene.h"

#include "GameObject/Camera/GameCamera.h"
#include "3d/Light/LightManager.h"
#include "GameObject/UI/StageStart/StageStart.h"
#include "GameObject/Character/Player/Player.h"
#include "GameObject/Camera/ClearCamera.h"
#include "GameObject/UI/GameUI/GameUI.h"
#include "scene/GameScene/State/GameSceneStateBase.h"
#include "GameObject/UI/Menu/MenuUI.h"
#include <memory>
#include "Input/InputContext.h"

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

	// ステートを切り替える
	void ChangeState(const std::string& stateName);

	void SetSceneTime(float time) { sceneTime_ = time; }

	Sprite* GetMuskSprite() { return musk_.get(); }
	float GetMuskAlpha() const { return muskAlpha_; }
	void SetMuskAlpha(float alpha) { muskAlpha_ = alpha; }

	// メニューのUIをまとめたクラスを取得
	MenuUI* GetMenuUI() { return menuUI_.get(); }
	// 入力の受付状態を管理するクラスを取得
	InputContext* GetInputContext() { return inputContext_.get(); }
private:
	std::unordered_map<std::string, std::unique_ptr<GameSceneStateBase>> states_;
	GameSceneStateBase* currentState_ = nullptr;
	// 入力をまとめたクラス
	std::unique_ptr<InputContext> inputContext_ = nullptr;

	CameraManager* cameraManager_ = CameraManager::GetInstance(); ///< カメラ管理クラス
	GameCamera* gameCamera_ = nullptr; ///< ゲームシーン専用カメラ
	ClearCamera* clearCamera_ = nullptr;

	LightManager* lightManager_ = LightManager::GetInstance(); ///< ライト管理クラス

	Player* player_ = nullptr; ///< プレイヤーオブジェクトへのポインタ



	std::unique_ptr<GameUI> gameUI_;

	// マスクを掛けるためのスプライト
	std::unique_ptr<Sprite> musk_ = nullptr;
	float muskAlpha_ = 0.0f;

	// メニューのスプライト
	std::unique_ptr<MenuUI> menuUI_ = nullptr;

	float sceneTime_ = 0.0f;
};
