#include "Object3d.h"
#include "Object3dManager.h"
#include "TextureManager.h"
//#include "imgui.h"
#include <WorldTransform.h>
#include <numbers>
//#include <Skeleton.h>
//#include <Animator.h>
#include "Model/ModelManager.h"

Object3d::Object3d(std::string objectName)
{
	name = objectName;
}

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
	if (ImGui::TreeNode("Transform")) {
		transform_->DebugGui();
		ImGui::TreePop();
	}

	for (size_t i = 0; i < renders_.size(); i++) {
		renders_[i]->DebugGui(i);
	}
}
#endif // _DEBUG

void Object3d::OnCollisionEnter(BaseCollider* other)
{
}

void Object3d::OnCollisionStay(BaseCollider* other)
{
}

void Object3d::OnCollisionExit(BaseCollider* other)
{
}

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

BaseCollider* Object3d::GetCollider(std::string name)
{
	for (auto& collider : colliders_) {
		if (collider->name == name) {
			return collider;
		}
	}
	Logger::Log("colliderが見つかりませんでした");
	return nullptr;
}
