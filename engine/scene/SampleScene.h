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
#include <ParticleEmitter.h>
#include "DebugSphere.h"
//#include <OffScreen.h>
#include "offscreen/GrayEffect.h"
#include <Light/LightManager.h>
#include <Renderer/ModelRenderer.h>
#include <Collider/AABBCollider.h>
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
	std::shared_ptr<Camera> normalCamera_;

	std::unique_ptr<Object3d> object_;
	std::unique_ptr<Object3d> object2_;
	//std::unique_ptr<Object3d> animationObject_;
	std::unique_ptr<Sprite> sprite;

	//Vector4 color1 = { 1.0f, 1.0f, 1.0f, 1.0f };
	//Vector4 color2 = { 1.0f, 1.0f, 1.0f, 1.0f };
	uint32_t soundHandle = 0u;

	//WorldTransform transform_;
	//WorldTransform animationTransform_;
	LightManager* lightManager_ = LightManager::GetInstance();

	//ParticleManager* particleManager_ = nullptr;
	std::unique_ptr<ParticleEmitter> particleEmitter_ = nullptr;

	DirectionalLight* dirLight_;

	std::unique_ptr<ModelRenderer> render1_;
	std::unique_ptr<ModelRenderer> render2_;


	Vector3 pos_{0.0f, -2.0f, 0.0f};
};

