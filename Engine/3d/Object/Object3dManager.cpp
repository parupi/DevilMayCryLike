#include "Object3dManager.h"
#include "3d/Object/Object3d.h"
#include <cassert>
#include <3d/Light/LightManager.h>
#include <3d/Camera/CameraManager.h>

Object3dManager* Object3dManager::instance = nullptr;
std::once_flag Object3dManager::initInstanceFlag;

Object3dManager* Object3dManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new Object3dManager();
		});
	return instance;
}

void Object3dManager::Initialize(DirectXManager* directXManager, PSOManager* psoManager)
{
	assert(directXManager);
	dxManager_ = directXManager;
	psoManager_ = psoManager;
}

void Object3dManager::Finalize()
{
	objects_.clear();

	dxManager_ = nullptr;
	psoManager_ = nullptr;
	defaultCamera_ = nullptr;

	delete instance;
	instance = nullptr;
}

void Object3dManager::Update()
{
	for (auto& object : objects_) {
		if (!object) continue;
		object->Update(deltaTime_);
	}
}

void Object3dManager::DrawSet()
{
	for (auto& object : objects_) {
		if (!object) continue;

		// ブレンドモードが違っていたら新しくPSOを設定
		if (blendMode_ != object->GetOption().blendMode) {
			// ブレンドモードを最新にしておく
			blendMode_ = object->GetOption().blendMode;

			dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetObjectPSO(blendMode_));			// PSOを設定
			dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetObjectSignature());
			dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// psoのセットをしたらlight,cameraもバインドしておく
			LightManager::GetInstance()->BindLightsToShader();
			CameraManager::GetInstance()->BindCameraToShader();
		}

		object->Draw();
	}


	// 次回の為にNoneに戻しておく
	blendMode_ = BlendMode::kNone;
}

void Object3dManager::DrawSetForAnimation()
{
	dxManager_->GetCommandList()->SetPipelineState(psoManager_->GetAnimationPSO());			// PSOを設定
	dxManager_->GetCommandList()->SetGraphicsRootSignature(psoManager_->GetAnimationSignature());
	dxManager_->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Object3dManager::AddObject(std::unique_ptr<Object3d> object)
{
	objects_.push_back(std::move(object));
}

void Object3dManager::DeleteObject(const std::string& name)
{
	for (auto& obj : objects_) {
		if (obj && obj->name_ == name) {
			obj->isAlive = false;
			obj->ResetObject();
		}
	}
}

void Object3dManager::DeleteAllObject()
{
	for (auto& obj : objects_) {
		// 全オブジェクトの生存フラグを切る
		obj->isAlive = false;
		obj->ResetObject();
	}
}

void Object3dManager::RemoveDeadObject()
{
	size_t before = objects_.size();

	objects_.erase(
		std::remove_if(objects_.begin(), objects_.end(),
			[](const std::unique_ptr<Object3d>& object) {
				return !object->isAlive;
			}),
		objects_.end()
	);

	size_t after = objects_.size();
	if (before != after) {
		char buf[128];
		sprintf_s(buf, "[Object3dManager] Removed %zu dead object(s)\n", before - after);
		OutputDebugStringA(buf);
	}
}


Object3d* Object3dManager::FindObject(std::string objectName)
{
	for (auto& object : objects_) {
		if (!object) continue;
		if (object->name_ == objectName) {
			return object.get();
		}
	}
	Logger::Log("not found renderer");
	return nullptr;
}

std::vector<Object3d*> Object3dManager::GetAllObject()
{
	std::vector<Object3d*> objects;

	for (auto& object : objects_) {
		if (!object) continue;
		objects.push_back(object.get());
	}

	return objects;
}

