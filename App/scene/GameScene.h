#pragma once
#include <3d/Object/Object3d.h>
#include <2d/Sprite.h>
#include <3d/Object/Model/Model.h>
#include <vector>
#include <3d/Camera/Camera.h>
#include <scene/BaseScene.h>
#include <memory>
#include <audio/Audio.h>
#include <3d/Camera/CameraManager.h>
#include <3d/WorldTransform.h>
#include <3d/Light/LightManager.h>
#include <base/Particle/ParticleEmitter.h>
#include <GameObject/Player/Player.h>
#include <GameObject/Enemy/Enemy.h>
#include <GameObject/Ground/Ground.h>
#include <GameObject/Camera/GameCamera.h>
#include <GameObject/StageStart/StageStart.h>

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
	//std::unique_ptr<GameCamera> gameCamera_;
	GameCamera* gameCamera_;
	                                                                                                           
	LightManager* lightManager_ = LightManager::GetInstance();

	// プレイヤーのポインタを持っておく
	Player* player_ = nullptr;

	StageStart stageStart_;
};

