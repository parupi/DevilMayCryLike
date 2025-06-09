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

void GameScene::Initialize()
{
	// カメラの生成
	normalCamera_ = std::make_shared<Camera>();
	cameraManager_->AddCamera(normalCamera_);
	cameraManager_->SetActiveCamera(0);
	normalCamera_->GetTranslate() = { 0.0f, 16.0f, -25.0f };
	normalCamera_->GetRotate() = { 0.5f, 0.0f, 0.0f };
	//normalCamera_->SetTranslate(Vector3{ 0.0f, 0.0f, -10.0f });
	//normalCamera_->SetRotate(Vector3{ 0.0f, 0.0f, 0.0f });



	// .objファイルからモデルを読み込む
	//ModelManager::GetInstance()->LoadModel("resource", "walk.gltf");
	//ModelManager::GetInstance()->LoadModel("resource", "simpleSkin.gltf");
	//ModelManager::GetInstance()->LoadModel("resource", "sneakWalk.gltf");
	//ModelManager::GetInstance()->LoadModel("resource", "plane.obj");
	//ModelManager::GetInstance()->LoadModel("resource", "AnimatedCube.gltf");
	//ModelManager::GetInstance()->LoadModel("resource", "terrain/terrain.obj");


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




	player_ = std::make_unique<Player>();

	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerBody"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerHead"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerLeftArm"));
	player_->AddRenderer(RendererManager::GetInstance()->FindRender("PlayerRightArm"));

	player_->Initialize();

	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("Enemy", "PlayerBody"));

	std::unique_ptr<AABBCollider> enemyCollider = std::make_unique<AABBCollider>("EnemyCollider");
	CollisionManager::GetInstance()->AddCollider(std::move(enemyCollider));


	enemy_ = std::make_unique<Enemy>();

	enemy_->AddRenderer(RendererManager::GetInstance()->FindRender("Enemy"));
	enemy_->AddCollider(CollisionManager::GetInstance()->FindCollider("EnemyCollider"));

	enemy_->Initialize();
	//animationObject_ = std::make_unique<Object3d>();
	//animationObject_->Initialize("walk.gltf");

	//transform_.Initialize();
	//animationTransform_.Initialize();

	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Ground", PrimitiveRenderer::PrimitiveType::Plane, "Terrain.png"));

	ground_ = std::make_unique<Ground>();

	ground_->AddRenderer(RendererManager::GetInstance()->FindRender("Ground"));

	ground_->Initialize();

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("fire", "circle.png");
	ParticleManager::GetInstance()->CreateParticleGroup("smork", "circle.png");




	//sprite = std::make_unique<Sprite>();
	//sprite->Initialize("resource/uvChecker.png");
	//sprite->SetSize({ 32.0f,32.0f });

	// ============ライト=================//

	std::unique_ptr<DirectionalLight> dirLight = std::make_unique<DirectionalLight>("Dir1");
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f , 1.0f};
	dirLight->GetLightData().intensity = 1.0f;
	//lightManager_->SetDirLightActive(0, true);
	lightManager_->AddDirectionalLight(std::move(dirLight));


}

void GameScene::Finalize()
{

}

void GameScene::Update()
{

	ParticleManager::GetInstance()->Update();

	//animationObject_->AnimationUpdate();
	cameraManager_->Update();
	//sprite->Update();
	lightManager_->UpdateAllLight();
	//Vector3 normalCameraPos = normalCamera_->GetTranslate();
	//Vector3 cameraRotate = normalCamera_->GetRotate();

	//ImGui::Begin("Camera Manager");
	//ImGui::DragFloat3("Translate", &normalCameraPos.x, 0.01f);
	//ImGui::DragFloat3("Rotate", &cameraRotate.x, 0.01f);
	//ImGui::End();

	//normalCamera_->SetTranslate(normalCameraPos);
	//normalCamera_->SetRotate(cameraRotate);

	
	//DebugUpdate();

	//Vector2 uvObjectPos = object_->GetUVPosition();
	//Vector2 uvObjectSize = object_->GetUVSize();
	//float uvObjectRotate = object_->GetUVRotation();

	//ImGui::Begin("Transform");
	//ImGui::DragFloat2("UVTranslate", &uvObjectPos.x, 0.01f, -10.0f, 10.0f);
	//ImGui::DragFloat2("UVScale", &uvObjectSize.x, 0.01f, -10.0f, 10.0f);
	//ImGui::SliderAngle("UVRotate", &uvObjectRotate);
	//ImGui::End();

	//Quaternion rotate = MakeRotateAxisAngleQuaternion(axis, angle);

	//PrintOnImGui(object_->GetWorldTransform()->GetMatWorld());

	//object_->SetUVPosition(uvObjectPos);
	//object_->SetUVSize(uvObjectSize);
	//object_->SetUVRotation(uvObjectRotate);


	player_->Update();

	enemy_->Update();

	ground_->Update();
	//animationObject_->Update();

	//ImGui::Begin("SetModel");
	//if (ImGui::Button("Set Work"))
	//{
	//	animationObject_->SetModel("walk.gltf");
	//}
	//if (ImGui::Button("Set sneakWalk"))
	//{
	//	animationObject_->SetModel("sneakWalk.gltf");
	//}
	//ImGui::End();

	//Vector2 spritePos = sprite->GetPosition();
	//Vector2 spriteSize = sprite->GetSize();
	//float spriteRotate = sprite->GetRotation();

	//Vector2 uvSpritePos = sprite->GetUVPosition();
	//Vector2 uvSpriteSize = sprite->GetUVSize();
	//float uvSpriteRotate = sprite->GetUVRotation();

	//ImGui::Begin("Sprite");
	//ImGui::DragFloat2("position", &spritePos.x);
	//ImGui::DragFloat("rotate", &spriteRotate);
	//ImGui::DragFloat2("size", &spriteSize.x);
	//ImGui::DragFloat2("UVTranslate", &uvSpritePos.x, 0.01f, -10.0f, 10.0f);
	//ImGui::DragFloat2("UVScale", &uvSpriteSize.x, 0.01f, -10.0f, 10.0f);
	//ImGui::SliderAngle("UVRotate", &uvSpriteRotate);
	//ImGui::End();

	//sprite->SetPosition(spritePos);
	//sprite->SetSize(spriteSize);
	//sprite->SetRotation(spriteRotate);

	//sprite->SetUVPosition(uvSpritePos);
	//sprite->SetUVSize(uvSpriteSize);
	//sprite->SetUVRotation(uvSpriteRotate);
}

void GameScene::Draw()
{


	//// 3Dオブジェクト描画前処理
	//Object3dManager::GetInstance()->DrawSetForAnimation();
	//lightManager_->BindLightsToShader();
	//animationObject_->Draw();

	Object3dManager::GetInstance()->DrawSet(BlendMode::kNormal);
	lightManager_->BindLightsToShader();
	cameraManager_->BindCameraToShader();

	ground_->Draw();
	player_->Draw();
	enemy_->Draw();


	Object3dManager::GetInstance()->DrawSet(BlendMode::kAdd);
	lightManager_->BindLightsToShader();
	cameraManager_->BindCameraToShader();
	player_->DrawEffect();
	enemy_->DrawEffect();

	ParticleManager::GetInstance()->DrawSet();
	ParticleManager::GetInstance()->Draw();
	//SpriteManager::GetInstance()->DrawSet();
	//sprite->Draw();
	
}

void GameScene::DrawRTV()
{
}

#ifdef _DEBUG
void GameScene::DebugUpdate()
{
	
	player_->DebugGui();
	
	ground_->DebugGui();

	enemy_->DebugGui();
}
#endif // _DEBUG
