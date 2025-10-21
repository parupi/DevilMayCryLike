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
	// シーン切り替え時のアニメーション起動
	void Exit();
	// シーン切り替え時のアニメーション更新
	void ExitUpdate();

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:

	void ChangePhase();

private:
	enum class TitlePhase {
		kFadeIn,
		kTitle,
		kUIAnimation,
		kFadeOut
	};
private:
	TitleCamera* camera_ = nullptr;
	CameraManager* cameraManager_ = CameraManager::GetInstance();
	//std::unique_ptr<Fade> fade_;

	// パーティクルのエミッター生成
	std::unique_ptr<ParticleEmitter> smokeEmitter_;
	std::unique_ptr<ParticleEmitter> smokeEmitter2_;
	std::unique_ptr<ParticleEmitter> sphereEmitter_;

	TitlePhase phase_ = TitlePhase::kFadeIn;

	// タイトルのUI群
	std::unique_ptr<Sprite> titleWord_;
	std::unique_ptr<Sprite> titleUnder_;
	std::unique_ptr<Sprite> titleUp_;

	// セレクトのUI群
	std::array<std::unique_ptr<Sprite>, 2> selectArrows_;
	std::unique_ptr<Sprite> gameStart_;
	std::unique_ptr<Sprite> selectMask_;

	bool isExit_ = false;
	float exitTime_ = 0.5f;
	float exitTimer_ = 0.0f;

	std::array<Vector2, 2> targetArrowSizes_;
	float targetSpriteAlpha_ = 1.0f;
	float targetSelectMaskAlpha = 0.0f;

	std::array<Vector2, 2> startArrowSizes_;
	float startSpriteAlpha_ = 1.0f;
	float startSelectMaskAlpha = 0.0f;

	LightManager* lightManager_ = LightManager::GetInstance();
	PointLight* pointLight_ = nullptr;
	DirectionalLight* directionLight_ = nullptr;

	Object3d* weaponObject_;

	float uiAnimationTimer_ = 0.0f;
	const float uiAnimationDuration_ = 0.3f; // 1秒間アニメーション

	SceneTransitionController* controller = SceneTransitionController::GetInstance();
};

