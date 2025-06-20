#include <GameScene.h>
#include <TextureManager.h>
#include <Particle/ParticleManager.h>
#include <imgui.h>
#include <Quaternion.h>
#include <Vector3.h>
#include <Matrix4x4.h>
#include <Model/ModelManager.h>
#include <Renderer/RendererManager.h>
#include <Renderer/ModelRenderer.h>
#include <Collider/CollisionManager.h>
#include <Renderer/PrimitiveRenderer.h>
#include <Include/SceneLoader.h>
#include <Include/SceneBuilder.h>

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
