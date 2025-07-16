#include "SampleScene.h"
#include <base/TextureManager.h>
#include <3d/Object/Model/ModelManager.h>
#include <base/Particle/ParticleManager.h>
#include <imgui/imgui.h>
#include <math/Quaternion.h>
#include <math/Vector3.h>
#include <math/Matrix4x4.h>
#include <offscreen/OffScreenManager.h>
#include <offscreen/VignetteEffect.h>
#include <offscreen/SmoothEffect.h>
#include <3d/Primitive/PrimitiveLineDrawer.h>
#include <3d/Object/Renderer/RendererManager.h>
#include <3d/Collider/CollisionManager.h>
#include <3d/Collider/SphereCollider.h>
#include <3d/Object/Renderer/PrimitiveRenderer.h>
#include <3d/SkySystem/SkySystem.h>

void SampleScene::Initialize()
{
	// カメラの生成
	normalCamera_ = std::make_unique<Camera>("GameCamera");
	normalCamera_->GetTranslate() = { 0.0f, 16.0f, -25.0f };
	normalCamera_->GetRotate() = { 0.5f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(normalCamera_));
	cameraManager_->SetActiveCamera(0);

	// .gltfファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadSkinnedModel("walk");
	ModelManager::GetInstance()->LoadSkinnedModel("simpleSkin");
	ModelManager::GetInstance()->LoadSkinnedModel("sneakWalk");
	//ModelManager::GetInstance()->LoadSkinnedModel("ParentKoala");
	//ModelManager::GetInstance()->LoadSkinnedModel("Characters_Anne");
	//ModelManager::GetInstance()->LoadSkinnedModel("BrainStem");
	// .objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("plane");
	ModelManager::GetInstance()->LoadModel("Terrain");
	ModelManager::GetInstance()->LoadModel("axis");
	ModelManager::GetInstance()->LoadModel("ICO");
	ModelManager::GetInstance()->LoadModel("multiMesh");
	ModelManager::GetInstance()->LoadModel("multiMaterial");
	ModelManager::GetInstance()->LoadModel("weapon");
	TextureManager::GetInstance()->LoadTexture("uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("gradationLine.png");

	TextureManager::GetInstance()->LoadTexture("Cube.png");
	TextureManager::GetInstance()->LoadTexture("rostock_laage_airport_4k.dds");
	TextureManager::GetInstance()->LoadTexture("circle.png");

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");

	// 天球の生成
	SkySystem::GetInstance()->CreateSkyBox("skybox_cube.dds");

	// オブジェクトを生成
	object_ = std::make_unique<Object3d>("obj1");
	object_->Initialize();
	object_->GetWorldTransform()->GetTranslation().y = -4.0f;


	// レンダラーの追加
	RendererManager::GetInstance()->AddRenderer(std::move(render1_));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render2", "Terrain"));
	// プリミティブレンダラーの生成、追加
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderPlane", PrimitiveType::Plane, "uvChecker.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderRing", PrimitiveType::Ring, "uvChecker.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("renderCylinder", PrimitiveType::Cylinder, "uvChecker.png"));

	render1_ = std::make_unique<PrimitiveRenderer>("renderPlane", PrimitiveType::Plane, "Terrain.png");

	render1_->GetWorldTransform()->GetScale() = { 1000.0f, 10.0f, 1000.0f };
	static_cast<Model*>(render1_->GetModel())->GetMaterials(0)->GetUVData().size.x = 100;
	static_cast<Model*>(render1_->GetModel())->GetMaterials(0)->GetUVData().size.y = 100;

	RendererManager::GetInstance()->AddRenderer(std::move(render1_));

	object_->AddRenderer(RendererManager::GetInstance()->FindRender("renderPlane"));
	// ゲームオブジェクトを追加
	Object3dManager::GetInstance()->AddObject(std::move(object_));

	// ============ライト=================//
	//lightManager_ = std::make_unique<LightManager>();
	std::unique_ptr<DirectionalLight> dirLight = std::make_unique<DirectionalLight>("dir1");
	dirLight->GetLightData().intensity = 1.0f;
	dirLight->GetLightData().enabled = true;
	dirLight->GetLightData().color = { 1.0f, 1.0f, 1.0f, 1.0f };
	dirLight->GetLightData().direction = { 0.0f, -1.0f, 0.0f };
	lightManager_->AddDirectionalLight(std::move(dirLight));

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");

	OffScreenManager::GetInstance()->AddEffect(std::make_unique<GrayEffect>());
	OffScreenManager::GetInstance()->AddEffect(std::make_unique<VignetteEffect>());
	OffScreenManager::GetInstance()->AddEffect(std::make_unique<SmoothEffect>());
}

void SampleScene::Finalize()
{
}

void SampleScene::Update()
{

	ParticleManager::GetInstance()->Update();

	player_->Update();

	cameraManager_->Update();
	lightManager_->UpdateAllLight();

	dirLight_ = lightManager_->GetDirectionalLight("dir1");
}

void SampleScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();

	Object3dManager::GetInstance()->DrawSetForAnimation();
	lightManager_->BindLightsToShader();
	cameraManager_->BindCameraToShader();
	player_->Draw();

	ParticleManager::GetInstance()->DrawSet();
	ParticleManager::GetInstance()->Draw();
}

void SampleScene::DrawRTV()
{


}

#ifdef _DEBUG
void SampleScene::DebugUpdate()
{
	//ImGui::Begin("Object");
	//object_->DebugGui();
	//ImGui::End();

	//ImGui::Begin("Object2");
	//object2_->DebugGui();
	//ImGui::End();
}
#endif
