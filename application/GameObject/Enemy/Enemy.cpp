#include "Enemy.h"
#include <Renderer/RendererManager.h>
#include <Renderer/PrimitiveRenderer.h>

Enemy::Enemy()
{
}

void Enemy::Initialize()
{
	Object3d::Initialize();

	GetWorldTransform()->GetTranslation() = { 0.0f, 0.0f, 5.0f };
	GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };
	static_cast<Model*>(GetRenderer("Enemy")->GetModel())->GetMaterials(0)->GetColor() = { 1.0f, 0.0f, 0.0f, 1.0f };

	particleEmitter_ = std::make_unique<ParticleEmitter>();
	particleEmitter_->Initialize("test");

	particleEmitter1_ = std::make_unique<ParticleEmitter>();
	particleEmitter1_->Initialize("fire");

	particleEmitter2_ = std::make_unique<ParticleEmitter>();
	particleEmitter2_->Initialize("smork");

	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Cylinder5", PrimitiveRenderer::PrimitiveType::Cylinder, "gradationLine_brightened.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Ring1", PrimitiveRenderer::PrimitiveType::Ring, "gradationLine_brightened.png"));
	RendererManager::GetInstance()->AddRenderer(std::make_unique<PrimitiveRenderer>("Ring2", PrimitiveRenderer::PrimitiveType::Ring, "gradationLine_brightened.png"));

	effect_ = std::make_unique<EnemyDamageEffect>();

	effect_->AddRenderer(RendererManager::GetInstance()->FindRender("Cylinder5"));
	effect_->AddRenderer(RendererManager::GetInstance()->FindRender("Ring1"));
	effect_->AddRenderer(RendererManager::GetInstance()->FindRender("Ring2"));

	effect_->Initialize();
}

void Enemy::Update()
{
	particleEmitter_->Update({ 0.0f, 0.0f, 0.0f }, 8);
	particleEmitter1_->Update({ 0.0f, 0.0f, 0.0f }, 8);
	particleEmitter2_->Update({ 0.0f, 0.0f, 0.0f }, 8);

	effect_->UpdateCameraShake(CameraManager::GetInstance()->GetActiveCamera().get());
	effect_->UpdateDamageRingEffect(GetWorldTransform()->GetTranslation());
	effect_->UpdateGroundRippleEffect(GetWorldTransform()->GetTranslation());
	effect_->Update();
	Object3d::Update();


}

void Enemy::Draw()
{


	Object3d::Draw();
}

void Enemy::DrawEffect()
{
	effect_->Draw();
}

#ifdef _DEBUG

void Enemy::DebugGui()
{
	ImGui::Begin("Enemy");
	Object3d::DebugGui();
	ImGui::End();

	ImGui::Begin("EnemyEffect");
	effect_->DebugGui();
	ImGui::End();
}
#endif // _DEBUG


void Enemy::OnCollisionEnter(BaseCollider* other)
{




	particleEmitter1_->Emit();


	effect_->InitializeCameraShake(CameraManager::GetInstance()->GetActiveCamera().get()->GetTranslate(), 0.5f, 0.2f, 30.0f);
	effect_->InitializeGroundRippleEffect(1.0f);
	effect_->InitializeDamageRingEffect(1.0f);
}

void Enemy::OnCollisionStay(BaseCollider* other)
{
	particleEmitter_->Emit();
}

void Enemy::OnCollisionExit(BaseCollider* other)
{
	particleEmitter2_->Emit();
	hp_--;

	if (hp_ <= 0) {
		//GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
	}
}
