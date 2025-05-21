#include "CameraManager.h"
#include <Object3dManager.h>
#include "ParticleManager.h"

CameraManager* CameraManager::instance = nullptr;
std::once_flag CameraManager::initInstanceFlag;

CameraManager* CameraManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new CameraManager();
		});
	return instance;
}

void CameraManager::Initialize()
{
	activeCameraIndex_ = 0;
}

void CameraManager::Finalize()
{
	cameras_.clear();
	delete instance;
	instance = nullptr;
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

void CameraManager::BindCameraToShader()
{

}
