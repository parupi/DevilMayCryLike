#include "SampleScene.h"
#include <TextureManager.h>
#include <Model/ModelManager.h>
#include <ParticleManager.h>
#include <imgui.h>
#include <Quaternion.h>
#include <Vector3.h>
#include <Matrix4x4.h>
#include <offscreen/OffScreenManager.h>
#include <offscreen/VignetteEffect.h>
#include <offscreen/SmoothEffect.h>
#include <Primitive/PrimitiveDrawer.h>
#include <Renderer/RendererManager.h>
#include <Collider/CollisionManager.h>
#include <Collider/SphereCollider.h>
#include <Renderer/PrimitiveRenderer.h>

void SampleScene::Initialize()
{
	// カメラの生成
	//normalCamera_ = std::make_shared<Camera>();
	//cameraManager_->AddCamera(normalCamera_);
	//cameraManager_->SetActiveCamera(0);
	//normalCamera_->GetTranslate() = { 0.0f, 35.0f, -44.0f };
	//normalCamera_->GetRotate() = { 0.68f, 0.0f, 0.0f };


	// .gltfファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadSkinnedModel("walk");
	ModelManager::GetInstance()->LoadSkinnedModel("simpleSkin");
	ModelManager::GetInstance()->LoadSkinnedModel("sneakWalk");
	ModelManager::GetInstance()->LoadSkinnedModel("ParentKoala");
	// .objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane");
	ModelManager::GetInstance()->LoadModel("Terrain");
	ModelManager::GetInstance()->LoadModel("axis");
	ModelManager::GetInstance()->LoadModel("ICO");
	ModelManager::GetInstance()->LoadModel("multiMesh");
	ModelManager::GetInstance()->LoadModel("multiMaterial");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine.png");

	object_ = std::make_unique<Object3d>("");
	object_->Initialize();

	object2_ = std::make_unique<Object3d>("");
	object2_->Initialize();
	//animationObject_ = std::make_unique<Object3d>();
	//animationObject_->Initialize("simpleSkin");

	render1_ = std::make_unique<ModelRenderer>("render1", "multiMaterial");
	//render1_->SetModel();
	//render2_ =;
	//render2_->SetModel("Terrain");

	RendererManager::GetInstance()->AddRenderer(std::move(render1_));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render2", "Terrain"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderPlane", PrimitiveRenderer::PrimitiveType::Plane, "uvChecker.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderRing", PrimitiveRenderer::PrimitiveType::Ring, "uvChecker.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderCylinder", PrimitiveRenderer::PrimitiveType::Cylinder, "uvChecker.png"));

	object_->AddRenderer(RendererManager::GetInstance()->FindRender("render1"));
	object_->AddRenderer(RendererManager::GetInstance()->FindRender("render2"));

	object2_->AddRenderer(RendererManager::GetInstance()->FindRender("renderPlane"));
	object2_->AddRenderer(RendererManager::GetInstance()->FindRender("renderRing"));
	object2_->AddRenderer(RendererManager::GetInstance()->FindRender("renderCylinder"));

	//transform_.Initialize();
	//animationTransform_.Initialize();

	//sprite = std::make_unique<Sprite>();
	//sprite->Initialize("Resource/uvChecker.png");
	//sprite->SetSize({ 32.0f,32.0f });

	// ============ライト=================//
	//lightManager_ = std::make_unique<LightManager>();
	std::unique_ptr<DirectionalLight> dirLight;
	dirLight = std::make_unique<DirectionalLight>("dir1");
	dirLight->GetLightData().intensity = 1.0f;
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	lightManager_->AddDirectionalLight(std::move(dirLight));

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");

	particleEmitter_ = std::make_unique<ParticleEmitter>();
	particleEmitter_->Initialize("test");

	//grayEffect_ = std::make_unique<GrayEffect>();
	//grayEffect_->Initialize();


	std::unique_ptr<SphereCollider> collider = std::make_unique<SphereCollider>("collider1");
	collider->Initialize();
	CollisionManager::GetInstance()->AddCollider(std::move(collider));

	std::unique_ptr<SphereCollider> collider2 = std::make_unique<SphereCollider>("collider2");
	collider2->Initialize();
	CollisionManager::GetInstance()->AddCollider(std::move(collider2));

	object_->AddCollider(CollisionManager::GetInstance()->FindCollider("collider1"));
	object2_->AddCollider(CollisionManager::GetInstance()->FindCollider("collider2"));

	OffScreenManager::GetInstance()->AddEfect(std::make_unique<GrayEffect>());
	OffScreenManager::GetInstance()->AddEfect(std::make_unique<VignetteEffect>());
	OffScreenManager::GetInstance()->AddEfect(std::make_unique<SmoothEffect>());
}

void SampleScene::Finalize()
{
}

void SampleScene::Update()
{
	particleEmitter_->Update({ 0.0f, 0.0f, 0.0f }, 8);
	ParticleManager::GetInstance()->Update();

	cameraManager_->Update();
	lightManager_->UpdateAllLight();
	//sprite->Update();

	dirLight_ = lightManager_->GetDirectionalLight("dir1");

	ImGui::Begin("Light");
	ImGui::DragFloat3("Direction", &dirLight_->GetLightData().direction.x, 0.01f);
	ImGui::End();



	ImGui::Begin("Camera Manager");
	ImGui::DragFloat3("Translate", &normalCamera_->GetTranslate().x, 0.01f);
	ImGui::DragFloat3("Rotate", &normalCamera_->GetRotate().x, 0.01f);
	ImGui::End();



	//DebugUpdate();


	object_->Update();
	object2_->Update();

	//animationObject_->Update();

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

	//ImGui::Begin("primitive");
	//ImGui::DragFloat3("position", &pos_.x, 0.01f);
	//ImGui::End();
}

void SampleScene::Draw()
{
	//ParticleManager::GetInstance()->DrawSet();
	//ParticleManager::GetInstance()->Draw();

	//// 3Dオブジェクト描画前処理
	//Object3dManager::GetInstance()->DrawSetForAnimation();
	//lightManager_->BindLightsToShader();
	//animationObject_->Draw();

	//Object3dManager::GetInstance()->DrawSet();
	//lightManager_->BindLightsToShader();
	//object_->Draw();
  
	// 3Dオブジェクト描画前処理
	//Object3dManager::GetInstance()->DrawSetForAnimation();
	//lightManager_->BindLightsToShader();
	//cameraManager_->BindCameraToShader();
	//animationObject_->Draw();

	//Object3dManager::GetInstance()->DrawSet(BlendMode::kNormal);
	//lightManager_->BindLightsToShader();
	//cameraManager_->BindCameraToShader();
	//object_->Draw();
	//object2_->Draw();
	//

	//SpriteManager::GetInstance()->DrawSet();
	//sprite->Draw();

	//PrimitiveDrawer::GetInstance()->DrawLine({ 0.0f, 0.0f, 0.0f }, { 0.0f, 5.0f, 0.0f }, {1.0f, 1.0f, 1.0f, 1.0f});
	//for (uint32_t i = 0; i < 30; i++) {
	//	Vector3 pos = pos_;
	//	pos.y += i;

	//	PrimitiveDrawer::GetInstance()->DrawRing(pos, 0.2f, 1.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, 32, RingDrawMode::Fill, "gradationLine.png");
	//	PrimitiveDrawer::GetInstance()->DrawCylinder(pos, 32, 0.5f, 0.5f, 3.0f, "gradationLine.png");
	//}

	//PrimitiveDrawer::GetInstance()->DrawRing({ 0.0f, 0.0f, 0.0f }, 0.1f, 0.3f, {1.0f, 1.0f, 1.0f, 1.0f}, 32, RingDrawMode::Line);
}

void SampleScene::DrawRTV()
{


}

#ifdef _DEBUG
void SampleScene::DebugUpdate()
{
	ImGui::Begin("Object");
	object_->DebugGui();
	ImGui::End();

	ImGui::Begin("Object2");
	object2_->DebugGui();
	ImGui::End();
}
#endif
