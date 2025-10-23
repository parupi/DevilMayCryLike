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

	std::unique_ptr<PointLight> pointLight2 = std::make_unique<PointLight>("Point2");
	pointLight2->GetLightData().enabled = true;
	pointLight2->GetLightData().position = { 0.0f, 2.0f, -6.0f };
	pointLight2->GetLightData().color = { 0.3f, 0.3f, 0.3f , 0.5f };
	pointLight2->GetLightData().intensity = 2.0f;
	pointLight2->GetLightData().radius = 10.0f;
	pointLight2->GetLightData().decay = 1.0f;
	lightManager_->AddPointLight(std::move(pointLight2));

	std::unique_ptr<DirectionalLight> dirLight = std::make_unique<DirectionalLight>("Dir1");
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f , 1.0f };
	dirLight->GetLightData().intensity = 0.1f;
	lightManager_->AddDirectionalLight(std::move(dirLight));

	smokeEmitter_ = std::make_unique<ParticleEmitter>();
	smokeEmitter_->Initialize("TitleSmoke");

	smokeEmitter2_ = std::make_unique<ParticleEmitter>();
	smokeEmitter2_->Initialize("TitleSmoke2");

	sphereEmitter_ = std::make_unique<ParticleEmitter>();
	sphereEmitter_->Initialize("TitleSphere");

	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Title.json"));

	titleUI_ = std::make_unique<TitleUI>();
	titleUI_->Initialize();

	// 新しいトランジションの追加 // 追加したら勝手に設定してくれる
	TransitionManager::GetInstance()->AddTransition(std::make_unique<FadeTransition>("Fade"));
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

	lightManager_->UpdateAllLight();

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
