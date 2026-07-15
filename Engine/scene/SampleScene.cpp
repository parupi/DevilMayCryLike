#include "SampleScene.h"
#include "Graphics/Resource/TextureManager.h"
#include <World3D/Object/Model/ModelManager.h>
#include <Graphics/Rendering/Particle/ParticleManager.h>
#include <imgui/imgui.h>
#include <Math/Quaternion.h>
#include <Math/Vector3.h>
#include <Math/Matrix4x4.h>
#include <Graphics/Rendering/PostEffect/OffScreenManager.h>
#include <Graphics/Rendering/PostEffect/VignetteEffect.h>
#include <Graphics/Rendering/PostEffect/SmoothEffect.h>
#include <World3D/Primitive/PrimitiveLineDrawer.h>
#include <World3D/Object/Renderer/RendererManager.h>
#include <World3D/Collider/CollisionManager.h>
#include <World3D/Collider/SphereCollider.h>
#include <World3D/Object/Renderer/PrimitiveRenderer.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include <Graphics/Rendering/PostEffect/GaussianEffect.h>
#include "Graphics/Rendering/Sprite/SpriteManager.h"

void SampleScene::Initialize() {
	//// カメラの生成
	//normalCamera_ = std::make_unique<BaseCamera>("GameCamera");
	//normalCamera_->GetTranslate() = {0.0f, 15.0f, -30.0f};
	//normalCamera_->GetRotate() = {0.4f, 0.0f, 0.0f};
	//cameraManager_->AddCamera(std::move(normalCamera_));
	//cameraManager_->SetActiveCamera("GameCamera");

	//// カメラの生成
	//normalCamera_ = std::make_unique<BaseCamera>("KnockCamera");
	//normalCamera_->GetTranslate() = {0.0f, 0.0f, -10.0f};
	//normalCamera_->GetRotate() = {0.0f, 0.0f, 0.0f};
	//cameraManager_->AddCamera(std::move(normalCamera_));
	//cameraManager_->SetActiveCamera("KnockCamera");

	//// .gltfファイルからモデルを読み込む
	//ModelManager::GetInstance().LoadSkinnedModel("walk");
	//ModelManager::GetInstance().LoadSkinnedModel("simpleSkin");
	//ModelManager::GetInstance().LoadSkinnedModel("sneakWalk");
	//ModelManager::GetInstance().LoadSkinnedModel("ParentKoala");
	//ModelManager::GetInstance().LoadSkinnedModel("Warrior");
	////ModelManager::GetInstance().LoadSkinnedModel("Characters_Anne");
	//ModelManager::GetInstance().LoadSkinnedModel("BrainStem");
	//// .objファイルからモデルを読み込む
	//ModelManager::GetInstance().LoadModel("plane");
	//ModelManager::GetInstance().LoadModel("Terrain");
	//ModelManager::GetInstance().LoadModel("Cube");
	//ModelManager::GetInstance().LoadModel("ICO");
	//ModelManager::GetInstance().LoadModel("multiMesh");
	//ModelManager::GetInstance().LoadModel("multiMaterial");
	//ModelManager::GetInstance().LoadModel("weapon");
	//TextureManager::GetInstance().LoadTexture("uvChecker.png");
	//TextureManager::GetInstance().LoadTexture("gradationLine.png");
	//TextureManager::GetInstance().LoadTexture("white.png");
	//TextureManager::GetInstance().LoadTexture("hitSmoke.png");

	////TextureManager::GetInstance().LoadTexture("Cube.png");
	//TextureManager::GetInstance().LoadTexture("circle.png");

	////ParticleManager::GetInstance().CreateParticleGroup("test", "circle.png");
	////ParticleManager::GetInstance().CreateEmitter("testEmitter");

	////// オブジェクトを生成
	////std::unique_ptr<Object3d> object = std::make_unique<Object3d>("obj1");
	////object->Initialize();

	////// レンダラーの追加
	////RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("render", "Cube"));

	////object->AddRenderer(RendererManager::GetInstance().FindRender("render"));

	////object->GetWorldTransform()->GetTranslation() = { 0.0f, 2.0f, 0.0f };

	//////object->GetOption().drawPath = DrawPath::Forward;
	//////object->GetRenderer("render")->GetModel()->GetMaterials()[1]->SetEnableTextureDensity(true);

	////object_ = object.get();
	////// ゲームオブジェクトを追加
	////Object3dManager::GetInstance().AddObject(std::move(object));

	////// オブジェクトを生成
	////object = std::make_unique<Object3d>("obj2");
	////object->Initialize();

	////// レンダラーの追加
	////RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>("render2", "Terrain"));

	////object->AddRenderer(RendererManager::GetInstance().FindRender("render2"));

	////object2_ = object.get();

	////Object3dManager::GetInstance().AddObject(std::move(object));

	//////sprite_ = SpriteManager::GetInstance().CreateSprite(SpriteLayer::Game, "test", "uvChecker.png");

	//////emitter_ = std::make_unique<ParticleEmitter>();
	//////emitter_->Initialize("test");
	//////emitter_->SetParticle("test");

	//ParticleManager::GetInstance().CreateParticleGroup("EnemyDamageEffect", "white.png");
	//ParticleManager::GetInstance().CreateParticleGroup("PlayerSlashEffect", "circle.png");

	//ParticleManager::GetInstance().CreateEmitter("HitEffect", "EnemyDamageEmitter");
	//auto& emitters = ParticleManager::GetInstance().GetEmitters();
	//emitter_ = emitters.at("HitEffect").get();
	////emitter_->SetParent(object_->GetWorldTransform());
	////emitter_->AddParticle("EnemyDamageEffect");
	////emitter_->AddParticle("PlayerSlashEffect");
	//emitter_->SetActiveFlag(false);

	//// ============ライト=================//
	//lightManager_->AddLight(std::make_unique<PointLight>("SamplePoint"));
	//lightManager_->AddLight(std::make_unique<DirectionalLight>("SampleDir"));
	//lightManager_->AddLight(std::make_unique<SpotLight>("SampleSpot"));
}

void SampleScene::Finalize() {
}

void SampleScene::Update() {
	//ImGui::Begin("SampleScene Debug Info");
	//ImGui::DragFloat2("uvSize", &Object3dManager::GetInstance().FindObject("obj1")->GetRenderer("render")->GetModel()->GetMaterials()[1]->GetUVData().size.x, 0.01f);
	//ImGui::End();

	//sprite_->Update();

	//emitter_->SetTranslate(object_->GetWorldTransform()->GetTranslation());

	//lightManager_->Update();

	//emitter_->Update();

#ifdef _DEBUG
	//DebugUpdate();
#endif // _DEBUG
}

void SampleScene::Draw() {
	//ParticleManager::GetInstance().DrawSet();
	ParticleManager::GetInstance().Draw();
}


#ifdef _DEBUG
void SampleScene::DebugUpdate() {
	//ImGui::Begin("Object");
	//object_->DebugGui();
	//ImGui::End();

	//ImGui::Begin("Object2");
	//object2_->DebugGui();
	//ImGui::End();
}
#endif
