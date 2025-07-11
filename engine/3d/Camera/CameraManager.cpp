#include "CameraManager.h"
#include <3d/Object/Object3dManager.h>
#include "base/Particle/ParticleManager.h"

CameraManager* CameraManager::instance = nullptr;
std::once_flag CameraManager::initInstanceFlag;

CameraManager* CameraManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new CameraManager();
		});
	return instance;
}

void CameraManager::Initialize(DirectXManager* dxManager)
{
	activeCameraIndex_ = 0;

	dxManager_ = dxManager;

	CreateCameraResource();
}

void CameraManager::Finalize()
{
	// カメラリストを解放（unique_ptr なので明示的に clear）
	cameras_.clear();

	// アクティブカメラインデックス初期化
	activeCameraIndex_ = -1;

	// カメラ用リソース解放
	if (cameraResource_) {
		cameraResource_.Reset();
		cameraData_ = nullptr;
	}

	// 外部参照のポインタをヌルに
	dxManager_ = nullptr;

	delete instance;
	instance = nullptr;

	Logger::Log("CameraManager finalized.\n");
}

void CameraManager::AddCamera(std::unique_ptr<Camera> camera)
{
	cameras_.push_back(std::move(camera));
}

void CameraManager::Update()
{
	// アクティブカメラを更新
	if (activeCameraIndex_ < 0 || activeCameraIndex_ >= cameras_.size())	return;

	cameras_[activeCameraIndex_]->Update();
	cameraData_->worldPosition = cameras_[activeCameraIndex_]->GetTranslate();
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

Camera* CameraManager::GetActiveCamera() const
{
	if (activeCameraIndex_ >= 0 && activeCameraIndex_ < cameras_.size())
	{
		return cameras_[activeCameraIndex_].get();
	}
	return nullptr;
}

void CameraManager::BindCameraToShader()
{
	// cameraの場所を指定
	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
}

void CameraManager::CreateCameraResource()
{
	// カメラ用のリソースを作る
	dxManager_->CreateBufferResource(sizeof(CameraForGPU), cameraResource_);
	// 書き込むためのアドレスを取得
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
	// 初期値を入れる
	cameraData_->worldPosition = { 1.0f, 1.0f, 1.0f };

	// ログ出力
	Logger::LogBufferCreation("Camera", cameraResource_.Get(), sizeof(CameraForGPU));
}
