#include "CameraManager.h"
#include <Object3dManager.h>
#include "ParticleManager.h"

CameraManager::CameraManager() : activeCameraIndex_(0) // 初期カメラのインデックスを0とする
{
}

CameraManager::~CameraManager()
{
	cameras_.clear();
}

void CameraManager::AddCamera(std::shared_ptr<Camera> camera)
{
	cameras_.emplace_back(camera);
}

void CameraManager::Update()
{
	// アクティブカメラを更新
	if (activeCameraIndex_ >= 0 && activeCameraIndex_ < cameras_.size())
	{
		cameras_[activeCameraIndex_]->Update();
	}
}

void CameraManager::SetActiveCamera(int index)
{
	if (index >= 0 && index < cameras_.size())
	{
		activeCameraIndex_ = index;
		Object3dManager::GetInstance()->SetDefaultCamera(cameras_[activeCameraIndex_].get());
		ParticleManager::GetInstance()->SetCamera(cameras_[activeCameraIndex_].get());
	}
}

std::shared_ptr<Camera> CameraManager::GetActiveCamera() const
{
	if (activeCameraIndex_ >= 0 && activeCameraIndex_ < cameras_.size())
	{
		return cameras_[activeCameraIndex_];
	}
	return nullptr;
}
