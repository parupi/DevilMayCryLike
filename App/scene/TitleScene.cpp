#include "TitleScene.h"
#include <base/TextureManager.h>
#include <2d/SpriteManager.h>
#include <input/Input.h>
#include <Include/SceneLoader.h>
#include <Include/SceneBuilder.h>
#include <3d/Object/Model/ModelManager.h>
#include <3d/SkySystem/SkySystem.h>
#include <3d/Light/PointLight.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <scene/Transition/FadeTransition.h>
#include <scene/Transition/TransitionManager.h>
#include <GameObject/Camera/TitleCamera.h>


void TitleScene::Initialize()
{
	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("weapon");
	ModelManager::GetInstance()->LoadModel("Sword");
	ModelManager::GetInstance()->LoadModel("Cube");
	TextureManager::GetInstance()->LoadTexture("white.png");
	TextureManager::GetInstance()->LoadTexture("Title.png");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("circle.png");
	TextureManager::GetInstance()->LoadTexture("circle2.png");
	TextureManager::GetInstance()->LoadTexture("TitleUnder.png");
	TextureManager::GetInstance()->LoadTexture("TitleUp.png");
	TextureManager::GetInstance()->LoadTexture("SelectArrow.png");
	TextureManager::GetInstance()->LoadTexture("GameStart.png");
	TextureManager::GetInstance()->LoadTexture("black.png");
	TextureManager::GetInstance()->LoadTexture("SelectMask.png");

	// カメラの生成
	
	cameraManager_->AddCamera(std::make_unique<TitleCamera>("TitleCamera"));
	cameraManager_->SetActiveCamera("TitleCamera");

	camera_ = static_cast<TitleCamera*>(cameraManager_->GetActiveCamera());
	camera_->Initialize();
	camera_->Enter();

	// タイトルシーンにあるもやもやを生成
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSphere", "circle2.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("TitleSmoke2", "smoke.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("qwantani_moon_noon_puresky_4k.dds");

	lightManager_->AddLight(std::make_unique<PointLight>("TitlePoint"));
	lightManager_->AddLight(std::make_unique<SpotLight>("TitleSpot"));
	lightManager_->AddLight(std::make_unique<DirectionalLight>("TitleDir"));

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("TitleSmoke");

	smokeEmitter2_ = std::make_unique<ParticleEmitter>();
	smokeEmitter2_->Initialize("TitleSmoke2");

	sphereEmitter_ = std::make_unique<ParticleEmitter>();
	sphereEmitter_->Initialize("TitleSphere");

	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Title.json"));

	titleUI_ = std::make_unique<TitleUI>();
	titleUI_->Initialize();

	// 新しいトランジションの追加
	TransitionManager::GetInstance()->AddTransition(std::make_unique<FadeTransition>("Fade"));
	TransitionManager::GetInstance()->SetTransition("Fade");
}

void TitleScene::Finalize()
{
	Object3dManager::GetInstance()->DeleteAllObject();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();
}

void TitleScene::Update()
{
	smokeEmitter_->Update();
	smokeEmitter2_->Update();

	sphereEmitter_->Update();

	lightManager_->Update();

	titleUI_->Update();

	ChangePhase();

#ifdef _DEBUG
	DebugUpdate();
#endif // _DEBUG

}

void TitleScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();

	ParticleManager::GetInstance()->Draw();

	titleUI_->Draw();
}

void TitleScene::DrawRTV()
{
}

#ifdef _DEBUG
void TitleScene::DebugUpdate()
{

}
#endif // _DEBUG

void TitleScene::ChangePhase()
{
	if (!camera_->IsExit()) {
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
			camera_->Exit();
			titleUI_->Exit();
		}
	}
}
