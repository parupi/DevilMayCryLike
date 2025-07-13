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
#include <base/Particle/ParticleEmitter.h>
#include "offscreen/GrayEffect.h"
#include <3d/Light/LightManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/Collider/AABBCollider.h>
#include "GameObject/Player/Player.h"
#include "GameObject/Camera/GameCamera.h"
class SampleScene : public BaseScene
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
	// RTV描画
	void DrawRTV() override;

#ifdef _DEBUG
	void DebugUpdate() override;
#endif // _DEBUG

private:
	CameraManager* cameraManager_ = CameraManager::GetInstance();
	std::unique_ptr<GameCamera> normalCamera_;

	LightManager* lightManager_ = LightManager::GetInstance();

	std::unique_ptr<Object3d> object_;
	std::unique_ptr<Object3d> weapon_;
	std::unique_ptr<Player> player_;


	std::unique_ptr<Sprite> sprite;

	//Vector4 color1 = { 1.0f, 1.0f, 1.0f, 1.0f };
	//Vector4 color2 = { 1.0f, 1.0f, 1.0f, 1.0f };
	uint32_t soundHandle = 0u;


	//ParticleManager* particleManager_ = nullptr;


	DirectionalLight* dirLight_;

	std::unique_ptr<PrimitiveRenderer> render1_;
	std::unique_ptr<ModelRenderer> render2_;


	Vector3 pos_{0.0f, -2.0f, 0.0f};
};

