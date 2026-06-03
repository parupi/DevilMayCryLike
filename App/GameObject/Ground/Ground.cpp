#include "Ground.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <memory>
#include "World3D/Object/Renderer/ModelRenderer.h"
#include <World3D/Collider/CollisionManager.h>

Ground::Ground(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();
}

void Ground::Initialize()
{
	// レンダラーの生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "Cube"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name_));

	GetCollider(name_)->category_ = CollisionCategory::Ground;
	// uvサイズをオブジェクトの大きさに合わせる
	GetRenderer(name_)->GetModel()->GetMaterials()[1]->SetEnableTextureDensity(true);

	//GetCollider(name_)->transform_->GetScale() = GetWorldTransform()->GetScale();
	//CollisionManager::GetInstance()->FindCollider(name_);
	//smokeEmitter_ = std::make_unique<ParticleEmitter>();
	//smokeEmitter_->Initialize(name_ + "Smoke");
	//smokeEmitter_->SetParent(GetWorldTransform());
	//smokeEmitter_->SetParticle("GameSmoke");

	//circleEmitter_ = std::make_unique<ParticleEmitter>();
	//circleEmitter_->Initialize(name_ + "Circle");
	//circleEmitter_->SetParent(GetWorldTransform());
	//circleEmitter_->SetParticle("GameCircle");
}

void Ground::Update(float deltaTime)
{
	//smokeEmitter_->Update();
	//circleEmitter_->Update();

	Object3d::Update(deltaTime);
}

void Ground::Draw()
{
	Object3d::Draw();
}


#ifdef _DEBUG
void Ground::DebugGui()
{
	ImGui::Begin("Ground");
	Object3d::DebugGui();
	ImGui::End();
}
#endif // _DEBUG


