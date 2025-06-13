#include <GameScene.h>
#include <TextureManager.h>
#include <ParticleManager.h>
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
	normalCamera_ = std::make_shared<Camera>();
	cameraManager_->AddCamera(normalCamera_);
	cameraManager_->SetActiveCamera(0);
	normalCamera_->GetTranslate() = { 0.0f, 16.0f, -25.0f };
	normalCamera_->GetRotate() = { 0.5f, 0.0f, 0.0f };

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

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerBody", "PlayerBody"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerHead", "PlayerHead"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerLeftArm", "PlayerLeftArm"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("PlayerRightArm", "PlayerRightArm"));

	// コライダーの生成
	CollisionManager::GetInstance()->AddCollider(std::make_unique<AABBCollider>("Player"));

	sceneObjects_ = SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/stage.json"));


	player_ = std::make_unique<Player>("Player");

	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerBody"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerLeftArm"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerRightArm"));

	player_->AddCollider(CollisionManager::GetInstance()->FindCollider("Player"));
	player_->Initialize();

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("Enemy", "PlayerBody"));

	std::unique_ptr<AABBCollider> enemyCollider = std::make_unique<AABBCollider>("EnemyCollider");
	CollisionManager::GetInstance()->AddCollider(std::move(enemyCollider));


	enemy_ = std::make_unique<Enemy>("Enemy");

	enemy_->AddRenderer(RendererManager::GetInstance()->FindRender("Enemy"));
	enemy_->AddCollider(CollisionManager::GetInstance()->FindCollider("EnemyCollider"));

	enemy_->Initialize();

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
	for (auto& object : sceneObjects_) {
		delete object;
	}
}

void GameScene::Update()
{
	ParticleManager::GetInstance()->Update();

	cameraManager_->Update();
	lightManager_->UpdateAllLight();

	player_->Update();

	enemy_->Update();

	for (auto& object : sceneObjects_) {
		object->Update();
	}

	ImGui::Begin("Camera");
	ImGui::DragFloat3("Translate", &normalCamera_->GetTranslate().x, 0.01f);
	ImGui::DragFloat3("Rotate", &normalCamera_->GetRotate().x, 0.01f);
	ImGui::End();
	DebugUpdate();
}

void GameScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet(BlendMode::kNormal);
	lightManager_->BindLightsToShader();
	cameraManager_->BindCameraToShader();

	//ground_->Draw();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
	player_->Draw();
	enemy_->Draw();

	for (auto& object : sceneObjects_) {
		object->Draw();
	}

	Object3dManager::GetInstance()->DrawSet(BlendMode::kAdd);
	lightManager_->BindLightsToShader();
	cameraManager_->BindCameraToShader();
	player_->DrawEffect();
	enemy_->DrawEffect();

	ParticleManager::GetInstance()->DrawSet();
	ParticleManager::GetInstance()->Draw();
}

void GameScene::DrawRTV()
{
}

#ifdef _DEBUG
void GameScene::DebugUpdate()
{
	player_->DebugGui();
	
	enemy_->DebugGui();
}
#endif // _DEBUG
