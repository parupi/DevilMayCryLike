#pragma once
#include <Object3d.h>
#include <Sprite.h>
#include <Model.h>
#include <vector>
#include <Camera.h>
#include <BaseScene.h>
#include <memory>
#include <Audio.h>
#include <CameraManager.h>
#include <WorldTransform.h>
#include <LightManager.h>
#include <ParticleEmitter.h>
#include "DebugSphere.h"
#include <GameObject/Player/Player.h>

class GameScene : public BaseScene
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
	CameraManager cameraManager_;
	std::shared_ptr<Camera> normalCamera_;

	std::unique_ptr<LightManager> lightManager_;

	std::unique_ptr<Player> player_;

};

