#include "DebugSphere.h"
#include <ModelManager.h>

void DebugSphere::Initialize()
{
	ModelManager::GetInstance()->LoadModel("resource", "ICO.obj");
	object_ = std::make_unique<Object3d>();
	object_->Initialize("ICO.obj");

	transform_.Initialize();
}

void DebugSphere::Finalize()
{

}

void DebugSphere::Update(const Vector3& position)
{
	//transform_.scale_ = { 0.05f, 0.05f, 0.05f };
	//transform_.translation_ = position;
	//transform_.TransferMatrix();
}

void DebugSphere::Draw()
{
	object_->Draw();
}
