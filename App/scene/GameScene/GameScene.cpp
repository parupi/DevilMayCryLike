#include "GameScene.h"
#include "Graphics/Resource/TextureManager.h"
#include "Graphics/Rendering/Particle/ParticleManager.h"
#include "Debugger/ImGuiManager.h"
#include "Math/Quaternion.h"
#include "Math/Vector3.h"
#include "Math/Matrix4x4.h"
#include "World3D/Object/Model/ModelManager.h"
#include "World3D/Object/Renderer/RendererManager.h"
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Collider/CollisionManager.h"
#include "World3D/Object/Renderer/PrimitiveRenderer.h"
#include "Stage/SceneLoader.h"
#include "Stage/SceneBuilder.h"
#include "Graphics/Rendering/Sky/SkySystem.h"
#include "GameObject/Event/EventManager.h"
#include "Scene/Transition/TransitionManager.h"
#include "Scene/Transition/SceneTransitionController.h"
#include "Graphics/Rendering/Sprite/SpriteManager.h"
#include "GameObject/Camera/LockOnCamera.h"
#include "Scene/GameScene/State/GameSceneStatePlay.h"
#include "State/GameSceneStateMenu.h"
#include "State/GameSceneStateStart.h"
#include "State/GameSceneStateClear.h"
#include <GameObject/Character/Enemy/Enemy.h>
#include "World3D/Object/Object3dManager.h"

void GameScene::Initialize() {
	// ステートの生成
	states_["Start"] = std::make_unique<GameSceneStateStart>();
	states_["Play"] = std::make_unique<GameSceneStatePlay>();
	states_["Menu"] = std::make_unique<GameSceneStateMenu>();
	states_["Clear"] = std::make_unique<GameSceneStateClear>();
	currentState_ = states_["Start"].get();

	// 入力の受付状態を管理するクラス生成
	inputContext_ = std::make_unique<InputContext>();
	inputContext_->Initialize(Input::GetInstance());

	// 最初のシーンに入る処理
	currentState_->Enter(*this);

	// カメラの生成
	std::unique_ptr<GameCamera> camera = std::make_unique<GameCamera>("GameCamera");
	camera->GetTranslate() = { 0.096f, 13.4f, -20.0f };
	camera->GetRotate() = { 0.5f, -0.005f, 0.0f };
	gameCamera_ = camera.get();
	cameraManager_->AddCamera(std::move(camera));

	std::unique_ptr<LockOnCamera> lockOnCamera = std::make_unique<LockOnCamera>("LockOnCamera");

	// カメラの生成
	std::unique_ptr<ClearCamera> clearCamera = std::make_unique<ClearCamera>("ClearCamera");
	clearCamera_ = clearCamera.get();
	cameraManager_->AddCamera(std::move(clearCamera));

	ModelManager::GetInstance()->LoadModel("PlayerBody");
	ModelManager::GetInstance()->LoadModel("PlayerHead");
	ModelManager::GetInstance()->LoadModel("PlayerLeftArm");
	ModelManager::GetInstance()->LoadModel("PlayerRightArm");
	ModelManager::GetInstance()->LoadModel("weapon");
	ModelManager::GetInstance()->LoadModel("Cube");
	ModelManager::GetInstance()->LoadModel("suzannu");
	ModelManager::GetInstance()->LoadModel("Sword");
	ModelManager::GetInstance()->LoadModel("spirit_knight");
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
	TextureManager::GetInstance()->LoadTexture("circle.png");
	TextureManager::GetInstance()->LoadTexture("smoke.png");
	TextureManager::GetInstance()->LoadTexture("white.png");
	TextureManager::GetInstance()->LoadTexture("Heart.png");

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("smoke", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("hitSmoke", "hitSmoke.png");
	ParticleManager::GetInstance()->CreateParticleGroup("GameCircle", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("GameSmoke", "smoke.png");
	ParticleManager::GetInstance()->CreateParticleGroup("EnemyDamageEffect", "white.png");
	ParticleManager::GetInstance()->CreateParticleGroup("PlayerSlashEffect", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("EnemyChargeRing", "white.png", PrimitiveType::Ring);

	// スカイボックスを生成
	SkySystem::GetInstance()->CreateSkyBox("moonless_golf_4k.dds");

	// ステージの情報を読み込んで生成
	SceneBuilder::BuildScene(SceneLoader::Load("Resource/Stage/Stage1.json"));

	lightManager_->AddLight(std::make_unique<DirectionalLight>("GameDirectionalLight"));

	lockOnSystem_ = std::make_unique<LockOnSystem>();

	player_ = static_cast<Player*>(Object3dManager::GetInstance()->FindObject("Player"));
	player_->SetInput(inputContext_->GetPlayerInput());
	player_->SetLockOn(lockOnSystem_.get());

	lockOnSystem_->Initialize(inputContext_->GetLockOnInput(), player_);

	// 生成された敵をロックオン対象に設定する
	for (auto* obj : Object3dManager::GetInstance()->GetAllObject()) {
		if (auto* enemy = dynamic_cast<Enemy*>(obj)) {
			enemy->SetupLockOn(lockOnSystem_.get());
		}
	}

	// カメラにプレイヤーとロックオンを受け渡す
	lockOnCamera->Initialize(player_, lockOnSystem_.get());
	// 生成が終わったカメラをマネージャに渡す
	cameraManager_->AddCamera(std::move(lockOnCamera));

	gameCamera_->Initialize(player_, lockOnSystem_.get(), inputContext_->GetCameraInput());

	gameUI_ = std::make_unique<GameUI>();
	gameUI_->Initialize();

	musk_ = SpriteManager::GetInstance()->CreateSprite(SpriteLayer::Game, "menuMusk", "white.png");
	musk_->SetSize({ 1280.0f, 720.0f });
	musk_->SetColor({ 0.0f, 0.0f, 0.0f, 0.5f });

	menuUI_ = std::make_unique<MenuUI>();
	menuUI_->Initialize(this);

	//deathText_ = SpriteManager::GetInstance()->CreateSprite(SpriteLayer::Game, "DeathText", "DeathText.png");
	//deathText_->SetAnchorPoint({0.5f, 0.5f});
	//deathText_->SetPosition({ 640.0f, 120.0f });
}

void GameScene::Finalize() {
	SpriteManager::GetInstance()->DeleteAllSprite();
	Object3dManager::GetInstance()->DeleteAllObject();
	CollisionManager::GetInstance()->DeleteAllCollider();
	RendererManager::GetInstance()->DeleteAllRenderer();
	CameraManager::GetInstance()->DeleteAllCamera();
	LightManager::GetInstance()->DeleteAllLight();

	EventManager::GetInstance()->Finalize();
}

void GameScene::Update()
{
	//lightManager_->Update();
	gameUI_->Update();

	if (currentState_) {
		currentState_->Update(*this);
	}

	musk_->SetColor({ 0.0f, 0.0f, 0.0f, muskAlpha_ });
	musk_->Update();

	//deathText_->Update();

	menuUI_->Update();

	inputContext_->Update();

	lockOnSystem_->Update();

	Object3dManager::GetInstance()->SetDeltaTime(sceneTime_);

#ifdef _DEBUG
	//DebugUpdate();
#endif
}

void GameScene::Draw() {
	// スプライトの描画前処理
	//SpriteManager::GetInstance()->DrawSet();
	// プレイヤーのスプライト描画
	if (player_) {
		player_->DrawEffect();
	}

	//gameUI_->Draw();

	//musk_->Draw();

	//menuUI_->Draw();

	// 全パーティクルの描画
	ParticleManager::GetInstance()->Draw();
}

void GameScene::DrawRTV() {}

#ifdef _DEBUG
void GameScene::DebugUpdate() {
	if (player_) {
		player_->DebugGui();
	}

	//Object3dManager::GetInstance()->FindObject("HellKaina")->DebugGui();
}
#endif // _DEBUG

void GameScene::ChangeState(const std::string& stateName) {
	currentState_->Exit(*this);
	auto it = states_.find(stateName);
	if (it != states_.end()) {
		currentState_ = it->second.get();
		currentState_->Enter(*this);
	}
}