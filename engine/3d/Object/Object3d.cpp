#include "Object3d.h"
#include "Object3dManager.h"
#include "TextureManager.h"
//#include "imgui.h"
#include <WorldTransform.h>
#include <numbers>
//#include <Skeleton.h>
//#include <Animator.h>
#include "Model/ModelManager.h"
Object3d::~Object3d()
{
}

void Object3d::Initialize()
{

	objectManager_ = Object3dManager::GetInstance();

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();
}

void Object3d::Update()
{
	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->Update(transform_.get());
	}

	transform_->TransferMatrix();

	//model_->Update();

	camera_ = objectManager_->GetDefaultCamera();

	Matrix4x4 worldViewProjectionMatrix;
	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = transform_->GetMatWorld() * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = transform_->GetMatWorld();
	}

	transform_->SetMapWVP(worldViewProjectionMatrix);
	transform_->SetMapWorld(transform_->GetMatWorld());


}

void Object3d::Draw()
{
	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->Draw(transform_.get());
	}
}


#ifdef _DEBUG
void Object3d::DebugGui()
{
	//model_->DebugGui(this);
	if (ImGui::TreeNode("Transform")) {
		transform_->DebugGui();
		ImGui::TreePop();
	}

	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->DebugGui(i);
	}
}

void Object3d::OnCollisionEnter(BaseCollider* other)
{
}

void Object3d::OnCollisionStay(BaseCollider* other)
{
}

void Object3d::OnCollisionExit(BaseCollider* other)
{
}

#endif // _DEBUG

void Object3d::AddRenderer(BaseRenderer* render)
{
	renders_.push_back(render);
}

void Object3d::AddCollider(BaseCollider* collider)
{
	collider->SetOwner(this);
	colliders_.push_back(collider);
}

BaseRenderer* Object3d::GetRenderer(std::string name)
{
	for (auto& render : renders_) {
		if (render->name == name) {
			return render;
		}
	}
	Logger::Log("renderが見つかりませんでした");
	return nullptr;
}
