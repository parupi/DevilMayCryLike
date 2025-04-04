#pragma once
#include <Camera.h>
#include <BaseScene.h>
#include "SceneManager.h"
#include <memory>
#include <fade/Fade.h>
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "CameraManager.h"

class TitleScene : public BaseScene
{
public:
	// 初期化
	void Initialize() override;
	// 終了
	void Finalize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;

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
	CameraManager cameraManager_;
	std::unique_ptr<Fade> fade_;

	TitlePhase phase_ = TitlePhase::kFadeIn;

	//std::unique_ptr<ParticleManager> particleManager_ = nullptr;
	//std::unique_ptr<ParticleEmitter> snowEmitter_ = nullptr;
};

