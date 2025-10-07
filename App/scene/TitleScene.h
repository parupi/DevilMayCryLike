#pragma once
#include <3d/Camera/Camera.h>
#include <scene/BaseScene.h>
#include "scene/SceneManager.h"
#include <memory>

#include <3d/Camera/CameraManager.h>
#include "fade/Fade.h"
#include <3d/Light/LightManager.h>

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
	enum class TitlePhase {
		kFadeIn,
		kTitle,
		kUIAnimation,
		kFadeOut
	};
private:
	std::unique_ptr<Camera> camera_ = nullptr;
	CameraManager* cameraManager_ = CameraManager::GetInstance();
	std::unique_ptr<Fade> fade_;

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
	

	LightManager* lightManager_ = LightManager::GetInstance();

	Object3d* weaponObject_;

	float uiAnimationTimer_ = 0.0f;
	const float uiAnimationDuration_ = 0.3f; // 1秒間アニメーション

};

