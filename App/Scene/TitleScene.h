#pragma once
#include "World3D/Camera/BaseCamera.h"
#include <Scene/BaseScene.h>
#include "Scene/SceneManager.h"
#include <memory>
#include <World3D/Camera/CameraManager.h>
#include <World3D/Light/LightManager.h>
#include <Scene/Transition/SceneTransitionController.h>
#include <GameObject/Camera/TitleCamera.h>
#include <GameObject/UI/TitleUI/TitleUI.h>
#include <Graphics/Rendering/Particle/ParticleManager.h>

/// <summary>
/// タイトルシーンを管理するクラス  
/// 
/// ゲーム起動時に最初に表示されるタイトル画面の制御を行う。  
/// カメラ、UI、ライト、パーティクル、フェード、シーン遷移など、  
/// シーン全体に関わる初期化・更新・描画処理をまとめて管理する。
/// </summary>
class TitleScene : public BaseScene
{
public:
	TitleScene() = default;
	~TitleScene() = default;

	/// <summary>
	/// シーンの初期化処理  
	/// 各種ゲームオブジェクト、UI、カメラ、パーティクル、ライトなどを生成・初期化する。
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// シーンの終了処理  
	/// 保持しているリソースやオブジェクトを解放する。
	/// </summary>
	void Finalize() override;

	/// <summary>
	/// シーンの更新処理  
	/// 入力やUI状態を確認し、タイトル画面の遷移処理を制御する。
	/// </summary>
	void Update() override;

	/// <summary>
	/// シーンの描画処理  
	/// モデル・UI・エフェクトなどのレンダリングを行う。
	/// </summary>
	void Draw() override;

	/// <summary>
	/// RTVへの描画処理  
	/// シーンレンダリング結果をRender Target Viewに出力する。
	/// </summary>
	void DrawRTV() override;

#ifdef _DEBUG
	/// <summary>
	/// デバッグ用更新処理  
	/// ImGuiを利用したパラメータ調整や内部情報の可視化を行う。
	/// </summary>
	void DebugUpdate() override;
#endif // _DEBUG

private:
	/// <summary>
	/// シーンの状態を変更する  
	/// フェーズごとの制御（例：タイトル表示 → フェードアウト → 次シーン遷移）を管理する。
	/// </summary>
	void ChangePhase();

private:
	TitleCamera* camera_ = nullptr; ///< タイトルシーン専用カメラ
	CameraManager* cameraManager_ = &CameraManager::GetInstance(); ///< カメラ管理クラス

	// ==========================
	// パーティクルエフェクト
	// ==========================
	ParticleEmitter* smokeEmitter_;  ///< 煙パーティクルエミッター①
	ParticleEmitter* smokeEmitter2_; ///< 煙パーティクルエミッター②
	ParticleEmitter* sphereEmitter_; ///< 球状パーティクルエミッター

	// ==========================
	// ライト・遷移・UI
	// ==========================
	LightManager* lightManager_ = &LightManager::GetInstance(); ///< ライト管理クラス
	SceneTransitionController* controller = &SceneTransitionController::GetInstance(); ///< シーン遷移コントローラ
	std::unique_ptr<TitleUI> titleUI_; ///< タイトル画面のUI要素管理
};
