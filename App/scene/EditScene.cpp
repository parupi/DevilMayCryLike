#include "EditScene.h"
#include <GameObject/Ground/Ground.h>
#include <3d/Object/Model/ModelManager.h>
#include <GameObject/Camera/GameCamera.h>
#include <GameObject/Character/Player/Player.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/SkySystem/SkySystem.h>
#include <3d/Light/LightManager.h>
#include "3d/Object/Object3dManager.h"
#include "base/Particle/ParticleManager.h"

void EditScene::Initialize()
{
	// カメラの生成
	std::unique_ptr<GameCamera> gameCamera = std::make_unique<GameCamera>("GameCamera");
	gameCamera->GetTranslate() = { 0.096f, 13.4f, -20.0f };
	gameCamera->GetRotate() = { 0.5f, -0.005f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(gameCamera));
	CameraManager::GetInstance()->SetActiveCamera("GameCamera");

	std::unique_ptr<BaseCamera> normalCamera = std::make_unique<BaseCamera>("NormalCamera");
	normalCamera->GetTranslate() = { 0.0f, 0.0f, -10.0f };
	normalCamera->GetRotate() = { 0.0f, -0.0f, 0.0f };
	CameraManager::GetInstance()->AddCamera(std::move(normalCamera));
	CameraManager::GetInstance()->SetActiveCamera("NormalCamera");

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(Input::GetInstance());
	
	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("PlayerLeftArm");
	ModelManager::GetInstance()->LoadModel("PlayerRightArm");
	ModelManager::GetInstance()->LoadModel("weapon");
	ModelManager::GetInstance()->LoadModel("Cube");
	ModelManager::GetInstance()->LoadModel("suzannu");
	ModelManager::GetInstance()->LoadModel("Sword");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine.png");
	TextureManager::GetInstance()->LoadTexture("Terrain.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine_brightened.png");
	TextureManager::GetInstance()->LoadTexture("MagicEffect.png");
	TextureManager::GetInstance()->LoadTexture("portal.png");
	TextureManager::GetInstance()->LoadTexture("DeathText.png");
	TextureManager::GetInstance()->LoadTexture("GameUI.png");
	TextureManager::GetInstance()->LoadTexture("reticle.png");
	TextureManager::GetInstance()->LoadTexture("hitSmoke.png");
	TextureManager::GetInstance()->LoadTexture("UI/Arrow.png");
	TextureManager::GetInstance()->LoadTexture("UI/plus.png");
	TextureManager::GetInstance()->LoadTexture("UI/RBButton.png");
	TextureManager::GetInstance()->LoadTexture("UI/SticDown.png");
	TextureManager::GetInstance()->LoadTexture("UI/YButton.png");
	TextureManager::GetInstance()->LoadTexture("white.png");

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("moonless_golf_4k.dds");

	std::unique_ptr<AABBCollider> collider = std::make_unique<AABBCollider>("Ground");
	collider->GetColliderData().isActive = true;
	collider->GetColliderData().offsetMax = { 20.0f, 2.0f, 20.0f };
	collider->GetColliderData().offsetMin = { -20.0f, -2.0f, -20.0f };
	CollisionManager::GetInstance()->AddCollider(std::move(collider));

	std::unique_ptr<AABBCollider> playerCollider = std::make_unique<AABBCollider>("Player");
	CollisionManager::GetInstance()->AddCollider(std::move(playerCollider));

	std::unique_ptr<Ground> ground = std::make_unique<Ground>("Ground");
	ground->AddCollider(CollisionManager::GetInstance()->FindCollider("Ground"));
	ground->Initialize();
	ground->GetWorldTransform()->GetScale() = { 20.0f, 2.0f, 20.0f };
	ground->GetWorldTransform()->GetTranslation().y = -10.0f;
	Object3dManager::GetInstance()->AddObject(std::move(ground));

	std::unique_ptr<Player> player = std::make_unique<Player>("Player");
	player->AddCollider(CollisionManager::GetInstance()->FindCollider("Player"));
	player->Initialize();
	player->SetInput(inputContext_->GetPlayerInput());
	Object3dManager::GetInstance()->AddObject(std::move(player));

	//LightManager::GetInstance()->CreateDirectionalLight("Directional");
	//ParticleManager::GetInstance()->CreateParticleGroup("EnemyDamageEffect")
}

void EditScene::Finalize()
{
}

void EditScene::Update()
{
	//ParticleManager::GetInstance()->CreateParticleGroup("EnemyDamageEffect", "white.png");
	LightManager::GetInstance()->Update();
}

void EditScene::Draw()
{
	// 全オブジェクトの描画
	//Object3dManager::GetInstance()->Draw
}

void EditScene::DrawRTV()
{
}

#ifdef _DEBUG
void EditScene::DebugUpdate()
{
	Object3dManager::GetInstance()->FindObject("Ground")->DebugGui();
}
#endif // DEBUG
