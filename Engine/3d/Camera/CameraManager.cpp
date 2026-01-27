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
	// カメラ切り替え用のカメラを追加しておく
	transitionCamera_ = std::make_unique<BaseCamera>("TransitionCamera");
}

void CameraManager::Finalize()
{
	// カメラリストを解放（unique_ptr なので明示的に clear）
	cameras_.clear();
	activeCameraName_.clear();

	if (cameraResource_) {
		cameraResource_.Reset();
		cameraData_ = nullptr;
	}

	dxManager_ = nullptr;

	delete instance;
	instance = nullptr;

	Logger::Log("CameraManager finalized.\n");
}

void CameraManager::AddCamera(std::unique_ptr<BaseCamera> camera)
{
	if (cameras_.contains(camera->name_)) {
		Logger::Log("[CameraManager] Warning: Camera \"" + camera->name_ + "\" already exists. Replacing it.\n");
	}
	cameras_[camera->name_] = std::move(camera);
}

void CameraManager::Update()
{
	//for (auto& cam : cameras_) {
	//	cam.second->Update();
	//}

	BaseCamera* camera = nullptr;

	if (isTransitioning_) {
		TransitionUpdate();
		camera = transitionCamera_.get();
	} else {
		camera = GetActiveCamera();
		if (camera) {
			camera->Update();
		}
	}

	if (camera) {
		cameraData_->worldPosition = camera->GetTranslate();
		Object3dManager::GetInstance()->SetDefaultCamera(camera);
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

	// トランジションカメラをセット
	Object3dManager::GetInstance()->SetDefaultCamera(transitionCamera_.get());
	ParticleManager::GetInstance()->SetCamera(transitionCamera_.get());

	transitionCamera_->SetFovY(cameras_[activeCameraName_]->GetFovY());
	transitionCamera_->SetAspectRate(cameras_[activeCameraName_]->GetAspectRate());
	transitionCamera_->SetNearClip(cameras_[activeCameraName_]->GetNearClip());
	transitionCamera_->SetFarClip(cameras_[activeCameraName_]->GetFarClip());

	// 現在の位置と回転をセット
	transitionCamera_->GetTranslate() = startPos_;
	transitionCamera_->GetRotate() = startRot_;
	transitionCamera_->Update();

	Logger::Log("[CameraManager] Transition started from \"" + activeCameraName_ + "\" to \"" + nextCameraName_ + "\" (" + std::to_string(transitionTime) + "s)\n");
}

BaseCamera* CameraManager::GetActiveCamera() const
{
	if (isTransitioning_) {
		return transitionCamera_.get();
	}

	if (activeCameraName_.empty()) return nullptr;

	auto it = cameras_.find(activeCameraName_);
	if (it != cameras_.end()) {
		return it->second.get();
	}
	return nullptr;
}

BaseCamera* CameraManager::GetCurrentCamera() const
{
	if (isTransitioning_) {
		return transitionCamera_.get();
	}
	return GetActiveCamera();
}

void CameraManager::BindCameraToShader()
{
	// cameraの場所を指定
	dxManager_->GetCommandList()->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
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

	auto currentCam = cameras_[activeCameraName_].get();
	auto it = cameras_.find(nextCameraName_);
	auto nextCam = it->second.get();

	startPos_ = currentCam->GetTranslate();
	startRot_ = currentCam->GetRotate(); // Quaternion型想定
	endPos_ = nextCam->GetTranslate();
	endRot_ = nextCam->GetRotate();

	// 補間
	Vector3 interpPos = Lerp(startPos_, endPos_, t);
	Vector3 interpRot = Lerp(startRot_, endRot_, t);

	// 一時的なカメラに適用
	transitionCamera_->GetTranslate() = interpPos;
	transitionCamera_->GetRotate() = interpRot;

	transitionCamera_->Update();

	cameraData_->worldPosition = interpPos;

	// 終了判定
	if (t >= 1.0f) {
		isTransitioning_ = false;
		activeCameraName_ = nextCameraName_;
		nextCameraName_.clear();
		Object3dManager::GetInstance()->SetDefaultCamera(cameras_[activeCameraName_].get());
		ParticleManager::GetInstance()->SetCamera(cameras_[activeCameraName_].get());
		return;
	}
	// 次に設定するカメラも更新しておく
	cameras_[nextCameraName_]->Update();
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
