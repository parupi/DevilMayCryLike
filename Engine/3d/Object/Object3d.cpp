#include "Object3d.h"
#include "Object3dManager.h"
#include "base/TextureManager.h"
#include <3d/WorldTransform.h>
#include <numbers>
#include "Model/ModelManager.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI

Object3d::Object3d(std::string objectName)
{
	name_ = objectName;
	isAlive = true;
	Initialize();
}

void Object3d::Initialize()
{
	objectManager_ = Object3dManager::GetInstance();

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();

	camera_ = objectManager_->GetDefaultCamera();
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
	if (!isDraw) return;

	for (size_t i = 0; i < renders_.size(); i++) {
		// ComputeSkinningを前処理として呼ぶ（SkinnedModelなら）
		if (auto skinned = dynamic_cast<SkinnedModel*>(renders_[i]->GetModel())) {
			skinned->UpdateSkinningWithCS();
		}

		
		// グラフィックスパイプラインに戻す
		Object3dManager::GetInstance()->GetDxManager()->GetCommandList()->SetPipelineState(Object3dManager::GetInstance()->GetPsoManager()->GetObjectPSO(BlendMode::kNormal));
		Object3dManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootSignature(Object3dManager::GetInstance()->GetPsoManager()->GetObjectSignature());
		Object3dManager::GetInstance()->GetDxManager()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		renders_[i]->Draw();
	}
}

void Object3d::ResetObject()
{
	for (auto& collider : colliders_) {
		collider->isAlive = false;
		collider = nullptr;
	}
	for (auto& renderer : renders_) {
		renderer->isAlive = false;
		renderer = nullptr;
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
	other;
}

void Object3d::OnCollisionStay(BaseCollider* other)
{
	other;
}

void Object3d::OnCollisionExit(BaseCollider* other)
{
	other;
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
		if (render->name_ == name) {
			return render;
		}
	}
	Logger::Log("renderが見つかりませんでした");
	return nullptr;
}

BaseCollider* Object3d::GetCollider(std::string name)
{
	for (auto& collider : colliders_) {
		if (collider->name_ == name) {
			return collider;
		}
	}
	Logger::Log("colliderが見つかりませんでした");
	return nullptr;
}
