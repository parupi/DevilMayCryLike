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
#include <offscreen/GaussianEffect.h>

void SampleScene::Initialize()
{
	// カメラの生成
	normalCamera_ = std::make_unique<BaseCamera>("KnockCamera");
	normalCamera_->GetTranslate() = { 0.0f, 0.0f, -10.0f };
	normalCamera_->GetRotate() = { 0.0f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(normalCamera_));
	cameraManager_->SetActiveCamera("KnockCamera");

	// .gltfファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadSkinnedModel("walk");
	ModelManager::GetInstance()->LoadSkinnedModel("simpleSkin");
	ModelManager::GetInstance()->LoadSkinnedModel("sneakWalk");
	ModelManager::GetInstance()->LoadSkinnedModel("ParentKoala");
	ModelManager::GetInstance()->LoadSkinnedModel("Warrior");
	//ModelManager::GetInstance()->LoadSkinnedModel("Characters_Anne");
	ModelManager::GetInstance()->LoadSkinnedModel("BrainStem");
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
	TextureManager::GetInstance()->LoadTexture("circle.png");

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");

	// 天球の生成
	SkySystem::GetInstance()->CreateSkyBox("skybox_cube.dds");

	// オブジェクトを生成
	object_ = std::make_unique<Object3d>("obj1");
	object_->Initialize();

	// レンダラーの追加
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render", "plane"));

	object_->AddRenderer(RendererManager::GetInstance()->FindRender("render"));


	// ゲームオブジェクトを追加
	Object3dManager::GetInstance()->AddObject(std::move(object_));

	// ============ライト=================//
	lightManager_->CreateDirectionalLight("dir");
}

void SampleScene::Finalize()
{

}

void SampleScene::Update()
{
	lightManager_->UpdateAllLight();

#ifdef _DEBUG
	DebugUpdate();
#endif // _DEBUG
}

void SampleScene::Draw()
{
	Object3dManager::GetInstance()->DrawSet();

	ParticleManager::GetInstance()->DrawSet();
	ParticleManager::GetInstance()->Draw();
}

void SampleScene::DrawRTV()
{


}

#ifdef _DEBUG
void SampleScene::DebugUpdate()
{
}
#endif
