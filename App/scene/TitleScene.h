#pragma once
#include <3d/Camera/Camera.h>
#include <scene/BaseScene.h>
#include "scene/SceneManager.h"
#include <memory>
#include <3d/Camera/CameraManager.h>
#include "fade/Fade.h"
#include <3d/Light/LightManager.h>
#include <scene/Transition/SceneTransitionController.h>
#include <GameObject/Camera/TitleCamera.h>
#include <GameObject/UI/TitleUI/TitleUI.h>

class TitleScene : public BaseScene
{
public:
	TitleScene() = default;
	~TitleScene() = default;

	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	void DrawRTV() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:

	void ChangePhase();

private:
	TitleCamera* camera_ = nullptr;
	CameraManager* cameraManager_ = CameraManager::GetInstance();

	// パーティクルのエミッター生成
	std::unique_ptr<ParticleEmitter> smokeEmitter_;
	std::unique_ptr<ParticleEmitter> smokeEmitter2_;
	std::unique_ptr<ParticleEmitter> sphereEmitter_;

	LightManager* lightManager_ = LightManager::GetInstance();

	SceneTransitionController* controller = SceneTransitionController::GetInstance();

	std::unique_ptr<TitleUI> titleUI_;
};

