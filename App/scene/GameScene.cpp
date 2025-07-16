#include <scene/GameScene.h>
#include <base/TextureManager.h>
#include <base/Particle/ParticleManager.h>
#include "debuger/ImGuiManager.h"
#include <math/Quaternion.h>
#include <math/Vector3.h>
#include <math/Matrix4x4.h>
#include <3d/Object/Model/ModelManager.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <Include/SceneLoader.h>
#include <Include/SceneBuilder.h>
#include <3d/SkySystem/SkySystem.h>

void GameScene::Initialize()
{
	// カメラの生成
	gameCamera_ = std::make_unique<GameCamera>("GameCamera");
	gameCamera_->GetTranslate() = { 0.0f, 16.0f, -25.0f };
	gameCamera_->GetRotate() = { 0.5f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(gameCamera_));
	cameraManager_->SetActiveCamera(0);


	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("PlayerLeftArm");
	ModelManager::GetInstance()->LoadModel("PlayerRightArm");
	ModelManager::GetInstance()->LoadModel("weapon");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("Terrain.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine_brightened.png");
	TextureManager::GetInstance()->LoadTexture("MagicEffect.png");

	// ステージの情報を読み込んで生成
	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/stage.json"));

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("smoke", "circle.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("skybox_cube.dds");

	// ============ライト=================//
	std::unique_ptr<DirectionalLight> dirLight = std::make_unique<DirectionalLight>("Dir1");
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f , 1.0f};
	dirLight->GetLightData().intensity = 1.0f;
	lightManager_->AddDirectionalLight(std::move(dirLight));
}

void GameScene::Finalize()
{
}

void GameScene::Update()
{


	lightManager_->UpdateAllLight();

	//player_->Update();

	//enemy_->Update();

	DebugUpdate();
}

void GameScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();
	//lightManager_->BindLightsToShader();
	//cameraManager_->BindCameraToShader();

	//ground_->Draw();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	//player_->Draw();
	//enemy_->Draw();

	//for (auto& object : sceneObjects_) {
	//	object->Draw();
	//}

	//Object3dManager::GetInstance()->DrawSet();
	//lightManager_->BindLightsToShader();
	//cameraManager_->BindCameraToShader();
	//player_->DrawEffect();
	//enemy_->DrawEffect();

	ParticleManager::GetInstance()->DrawSet();
	ParticleManager::GetInstance()->Draw();
}

void GameScene::DrawRTV()
{
}

#ifdef _DEBUG
void GameScene::DebugUpdate()
{
	//player_->DebugGui();
	
	//enemy_->DebugGui();
}
#endif // _DEBUG
