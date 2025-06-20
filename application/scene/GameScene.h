#pragma once
#include <Object3d.h>
#include <Sprite.h>
#include <Model/Model.h>
#include <vector>
#include <Camera.h>
#include <BaseScene.h>
#include <memory>
#include <Audio.h>
#include <CameraManager.h>
#include <WorldTransform.h>
#include <Light/LightManager.h>
#include <Particle/ParticleEmitter.h>
#include "DebugSphere.h"
#include <GameObject/Player/Player.h>
#include <GameObject/Enemy/Enemy.h>
#include <GameObject/Ground/Ground.h>
#include <GameObject/Camera/GameCamera.h>

class GameScene : public BaseScene
{
public:
	GameScene() = default;
	~GameScene() = default;

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
	CameraManager* cameraManager_ = CameraManager::GetInstance();
	//std::shared_ptr<Camera> normalCamera_;
	std::unique_ptr<GameCamera> gameCamera_;
	                                                                                                           
	LightManager* lightManager_ = LightManager::GetInstance();
};

