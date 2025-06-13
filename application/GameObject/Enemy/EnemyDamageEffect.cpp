#include "EnemyDamageEffect.h"

EnemyDamageEffect::EnemyDamageEffect(std::string objectName) : Object3d(objectName)
{
}

void EnemyDamageEffect::Initialize()
{
	Object3d::Initialize();

	GetRenderer("Ring1")->GetWorldTransform()->GetRotation() = EulerDegree(Vector3{ 20.0f, 0.0f, 10.0f });
	GetRenderer("Ring2")->GetWorldTransform()->GetRotation() = EulerDegree(Vector3{ -20.0f, 0.0f, 10.0f });

	GetRenderer("Ring1")->GetWorldTransform()->GetScale().x = 0.0f;
	GetRenderer("Ring1")->GetWorldTransform()->GetScale().z = 0.0f;

	GetRenderer("Ring2")->GetWorldTransform()->GetScale().x = 0.0f;
	GetRenderer("Ring2")->GetWorldTransform()->GetScale().z = 0.0f;

	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale() = { 0.0f, 0.0f, 0.0f };
	GetRenderer("Cylinder5")->GetWorldTransform()->GetTranslation().y = 0.0f;
	Object3d::Update();
}

void EnemyDamageEffect::InitializeGroundRippleEffect(float time)
{
	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale() = {1.0f, 3.0f, 1.0f};
	GetRenderer("Cylinder5")->GetWorldTransform()->GetTranslation().y = 0.0f;
	groundRippleData_.isActive = true;
}

void EnemyDamageEffect::InitializeDamageRingEffect(float time)
{

	GetRenderer("Ring1")->GetWorldTransform()->GetScale().x = 0.0f;
	GetRenderer("Ring1")->GetWorldTransform()->GetScale().z = 0.0f;

	GetRenderer("Ring2")->GetWorldTransform()->GetScale().x = 0.0f;
	GetRenderer("Ring2")->GetWorldTransform()->GetScale().z = 0.0f;

	damageRingData_.isActive = true;
}

void EnemyDamageEffect::Update()
{
	//auto colorPtr1 = &static_cast<Model*>(GetRenderer("Cylinder5")->GetModel())->GetMaterials(0)->GetColor();
	//auto colorPtr2 = &static_cast<Model*>(GetRenderer("Ring1")->GetModel())->GetMaterials(0)->GetColor();
	//if (colorPtr1 == colorPtr1) {
	//	int i;
	//	i = 0;
	//}

	Object3d::Update();
}

void EnemyDamageEffect::UpdateGroundRippleEffect(const Vector3& translate)
{
	if (!groundRippleData_.isActive) return;
	GetRenderer("Cylinder5")->GetWorldTransform()->GetTranslation() = translate;
	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().x += 0.5f;
	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().y -= 0.1f;
	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().z += 0.5f;
	static_cast<Model*>(GetRenderer("Cylinder5")->GetModel())->GetMaterials(0)->GetUVData().position.x += 0.01f;
	if (GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().y <= 0.0f) {
		groundRippleData_.isActive = false;
		GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().y = 0.0f;
	}
}

void EnemyDamageEffect::UpdateDamageRingEffect(const Vector3& translate)
{
	if (!damageRingData_.isActive) return;
	GetRenderer("Ring1")->GetWorldTransform()->GetTranslation() = translate;
	GetRenderer("Ring1")->GetWorldTransform()->GetScale().x += 0.8f;
	GetRenderer("Ring1")->GetWorldTransform()->GetScale().z += 0.8f;

	GetRenderer("Ring2")->GetWorldTransform()->GetTranslation() = translate;
	GetRenderer("Ring2")->GetWorldTransform()->GetScale().x += 0.8f;
	GetRenderer("Ring2")->GetWorldTransform()->GetScale().z += 0.8f;

	static_cast<Model*>(GetRenderer("Ring1")->GetModel())->GetMaterials(0)->GetUVData().position.x += 0.01f;
	static_cast<Model*>(GetRenderer("Ring2")->GetModel())->GetMaterials(0)->GetUVData().position.x += 0.01f;
	//if (GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().y <= 0.0f) {
	//	groundRippleData_.isActive = false;
	//	GetRenderer("Cylinder5")->GetWorldTransform()->GetScale().y = 0.0f;
	//}
}

void EnemyDamageEffect::Draw()
{
	Object3d::Draw();
}


#ifdef _DEBUG
void EnemyDamageEffect::DebugGui()
{
	Object3d::DebugGui();
}

#endif // _DEBUG

void EnemyDamageEffect::InitializeCameraShake(const Vector3& currentPos, float duration, float amplitude, float frequency)
{
	shake.isActive = true;
	shake.time = 0.0f;
	shake.duration = duration;
	shake.amplitude = amplitude;
	shake.frequency = frequency;
	shake.originalPosition = currentPos;
}

void EnemyDamageEffect::UpdateCameraShake(Camera* camera)
{
	if (!shake.isActive) return;

	shake.time += 1.0f / 60.0f;
	if (shake.time >= shake.duration) {
		shake.isActive = false;
		camera->GetTranslate() = shake.originalPosition;
		return;
	}

	float progress = shake.time / shake.duration;
	float decay = 1.0f - progress; // 揺れが徐々に減衰
	float shakeX = (std::sin(shake.time * shake.frequency * 2.0f * 3.14159f)) * shake.amplitude * decay;
	float shakeY = (std::cos(shake.time * shake.frequency * 2.0f * 3.14159f)) * shake.amplitude * decay;

	camera->GetTranslate() = shake.originalPosition + Vector3{ shakeX, shakeY, 0.0f };
}