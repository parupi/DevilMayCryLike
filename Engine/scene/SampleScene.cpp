#include "SampleScene.h"
#include "Graphics/Resource/TextureManager.h"
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
	normalCamera_ = std::make_unique<Camera>("GameCamera");
	normalCamera_->GetTranslate() = { 0.0f, 15.0f, -30.0f };
	normalCamera_->GetRotate() = { 0.4f, 0.0f, 0.0f };
	cameraManager_->AddCamera(std::move(normalCamera_));
	cameraManager_->SetActiveCamera("GameCamera");

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

	//TextureManager::GetInstance()->LoadTexture("Cube.png");
	TextureManager::GetInstance()->LoadTexture("circle.png");

	ParticleManager::GetInstance()->CreateParticleGroup("test", "circle.png");

	emitter_ = std::make_unique<ParticleEmitter>();
	emitter_->Initialize("test");

	// 天球の生成
	SkySystem::GetInstance()->CreateSkyBox("moonless_golf_4k.dds");

	// オブジェクトを生成
	std::unique_ptr<Object3d> object = std::make_unique<Object3d>("obj1");
	object->Initialize();

	// レンダラーの追加
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render", "axis"));

	object->AddRenderer(RendererManager::GetInstance()->FindRender("render"));

	//// モデルとアニメーション取得
	//SkinnedModel* model = static_cast<SkinnedModel*>(object->GetRenderer("render")->GetModel());
	//Animation* anim = model->GetAnimation();

	//anim->Play("Falling", true, 0.5f);

	//for (int32_t i = 0; i < 1; i++) {
	//	model->GetMaterials(i)->SetEnvironmentIntensity(1.0f);
	//}

	object_ = object.get();
	// ゲームオブジェクトを追加
	Object3dManager::GetInstance()->AddObject(std::move(object));

	// オブジェクトを生成
	object = std::make_unique<Object3d>("obj2");
	object->Initialize();

	// レンダラーの追加
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render2", "Terrain"));

	object->AddRenderer(RendererManager::GetInstance()->FindRender("render2"));

	//object->GetOption().drawPath = DrawPath::Forward;

	object2_ = object.get();

	Object3dManager::GetInstance()->AddObject(std::move(object));


	//// オブジェクトを生成
	//object = std::make_unique<Object3d>("obj3");
	//object->Initialize();

	//// レンダラーの追加
	//RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>("render3", "weapon"));

	//object->AddRenderer(RendererManager::GetInstance()->FindRender("render3"));

	//object->GetOption().drawPath = DrawPath::Deferred;

	//Object3dManager::GetInstance()->AddObject(std::move(object));


	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize("uvChecker.png");

	// ============ライト=================//
	lightManager_->AddLight(std::make_unique<DirectionalLight>("SampleDir"));
	lightManager_->AddLight(std::make_unique<PointLight>("SamplePoint"));
	lightManager_->AddLight(std::make_unique<SpotLight>("SampleSpot"));

	OffScreenManager::GetInstance()->AddEffect(std::make_unique<GrayEffect>());
	//OffScreenManager::GetInstance()->AddEffect(std::make_unique<VignetteEffect>());
	OffScreenManager::GetInstance()->AddEffect(std::make_unique<SmoothEffect>());
	OffScreenManager::GetInstance()->AddEffect(std::make_unique<GaussianEffect>());
}

void SampleScene::Finalize()
{
}

void SampleScene::Update()
{

	sprite_->Update();
	emitter_->Update();


	lightManager_->Update();

	//dirLight_ = lightManager_->GetDirectionalLight("dir1");

#ifdef _DEBUG
	DebugUpdate();
#endif // _DEBUG
}

void SampleScene::Draw()
{
	//Object3dManager::GetInstance()->DrawSet();
	//Object3dManager::GetInstance()->DrawForGBuffer();

	//RendererManager::GetInstance()->RenderGBufferPass();

	//Object3dManager::GetInstance()->DrawSetForAnimation();
	//lightManager_->BindLightsToShader();
	//cameraManager_->BindCameraToShader();


	//ParticleManager::GetInstance()->DrawSet();
	//ParticleManager::GetInstance()->Draw();
	//SpriteManager::GetInstance()->DrawSet();
	//sprite_->Draw();
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
