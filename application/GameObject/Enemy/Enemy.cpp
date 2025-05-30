#include "Enemy.h"

Enemy::Enemy()
{
}

void Enemy::Initialize()
{
	Object3d::Initialize();

	GetWorldTransform()->GetTranslation() = { 0.0f, 0.0f, 5.0f };
	GetWorldTransform()->GetScale() = { 0.8f, 0.8f, 0.8f };
	static_cast<Model*>(GetRenderer("Enemy")->GetModel())->GetMaterials(0)->GetColor() = { 1.0f, 0.0f, 0.0f, 1.0f };
}

void Enemy::Update()
{
	Object3d::Update();
}

void Enemy::Draw()
{
	Object3d::Draw();
}

void Enemy::DrawEffect()
{
}

void Enemy::DebugGui()
{

}
