#include "CameraManager.h"
#include <3d/Object/Object3dManager.h>
#include "base/Particle/ParticleManager.h"
#include <base/utility/DeltaTime.h>

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
	dxManager_ = dxManager;

	CreateCameraResource();

	activeCameraName_.clear();
}

void CameraManager::Finalize()
{
	// カメラリストを解放（unique_ptr なので明示的に clear）
	cameras_.clear();
	activeCameraName_.clear();

	cameraData_ = nullptr;

	dxManager_ = nullptr;

	delete instance;
	instance = nullptr;

	Logger::Log("CameraManager finalized.\n");
}

void CameraManager::AddCamera(std::unique_ptr<Camera> camera)
{
	if (cameras_.contains(camera->name_)) {
		Logger::Log("[CameraManager] Warning: Camera \"" + camera->name_ + "\" already exists. Replacing it.\n");
	}
	cameras_[camera->name_] = std::move(camera);
}

void CameraManager::Update()
{
	if (isTransitioning_)
	{
		TransitionUpdate();
	}

	if (auto it = cameras_.find(activeCameraName_); it != cameras_.end())
	{
		Camera* activeCamera = it->second.get();
		activeCamera->Update();
		cameraData_->worldPosition = activeCamera->GetTranslate();
	}
}

void CameraManager::SetActiveCamera(const std::string& cameraName, float transitionTime)
{
	auto it = cameras_.find(cameraName);
	if (it == cameras_.end()) {
		Logger::Log("[CameraManager] Error: Camera \"" + cameraName + "\" not found.\n");
		return;
	}

	// 補間時間が指定されていない or 0 の場合は即切り替え
	if (transitionTime <= 0.0f || activeCameraName_.empty()) {
		activeCameraName_ = cameraName;
		Object3dManager::GetInstance()->SetDefaultCamera(it->second.get());
		ParticleManager::GetInstance()->SetCamera(it->second.get());
		return;
	}

	// 補間初期化
	isTransitioning_ = true;
	transitionTime_ = transitionTime;
	transitionTimer_ = 0.0f;

	nextCameraName_ = cameraName;

	auto currentCam = cameras_[activeCameraName_].get();
	auto nextCam = it->second.get();

	startPos_ = currentCam->GetTranslate();
	startRot_ = currentCam->GetRotate(); // Quaternion型想定
	endPos_ = nextCam->GetTranslate();
	endRot_ = nextCam->GetRotate();

	Logger::Log("[CameraManager] Transition started from \"" + activeCameraName_ + "\" to \"" + nextCameraName_ + "\" (" + std::to_string(transitionTime) + "s)\n");
}

Camera* CameraManager::GetActiveCamera() const
{
	if (activeCameraName_.empty()) return nullptr;

	auto it = cameras_.find(activeCameraName_);
	if (it != cameras_.end()) {
		return it->second.get();
	}
	return nullptr;
}

void CameraManager::BindCameraToShader()
{
	// cameraの場所を指定
	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(1, dxManager_->GetResourceManager()->GetGPUVirtualAddress(cameraHandle_));
}

void CameraManager::DeleteAllCamera()
{
	cameras_.clear();
	activeCameraName_.clear();
}

void CameraManager::TransitionUpdate()
{
	// 経過時間を進める
	transitionTimer_ += DeltaTime::GetDeltaTime(); // ※Time::DeltaTime() はあなたのエンジン側の経過時間取得関数を使ってください

	// 0～1 の範囲に正規化
	float t = std::clamp(transitionTimer_ / transitionTime_, 0.0f, 1.0f);

	t = t * t * (3.0f - 2.0f * t);

	// 補間
	Vector3 interpPos = Lerp(startPos_, endPos_, t);
	Vector3 interpRot = Lerp(startRot_, endRot_, t);

	// 一時的なカメラに適用（アクティブカメラを動かす）
	auto activeCam = cameras_[activeCameraName_].get();
	activeCam->GetTranslate() = interpPos;
	activeCam->GetRotate() = interpRot;

	cameraData_->worldPosition = interpPos;

	// 終了判定
	if (t >= 1.0f)
	{
		isTransitioning_ = false;
		activeCameraName_ = nextCameraName_;
		nextCameraName_.clear();
	}
}

void CameraManager::CreateCameraResource()
{
	// カメラ用のリソースを作る
	cameraHandle_ = dxManager_->GetResourceManager()->CreateUploadBuffer(sizeof(CameraForGPU), L"CameraPos");
	// 書き込むためのアドレスを取得
	void* ptr = dxManager_->GetResourceManager()->Map(cameraHandle_);
	assert(ptr);
	cameraData_ = reinterpret_cast<CameraForGPU*>(ptr);
	// 初期値を入れる
	cameraData_->worldPosition = { 1.0f, 1.0f, 1.0f };
}
