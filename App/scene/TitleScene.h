#pragma once
#include <3d/Camera/Camera.h>
#include <scene/BaseScene.h>
#include "scene/SceneManager.h"
#include <memory>

#include <3d/Camera/CameraManager.h>
#include "fade/Fade.h"

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
		kFadeOut
	};
private:
	std::shared_ptr<Camera> camera_ = nullptr;
	CameraManager* cameraManager_ = CameraManager::GetInstance();
	std::unique_ptr<Fade> fade_;

	TitlePhase phase_ = TitlePhase::kFadeIn;

	//std::unique_ptr<ParticleManager> particleManager_ = nullptr;
	//std::unique_ptr<ParticleEmitter> snowEmitter_ = nullptr;
};

