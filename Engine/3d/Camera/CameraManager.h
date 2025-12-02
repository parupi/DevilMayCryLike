#pragma once
#include <map>
#include <memory>
#include "Camera.h"
#include <mutex>
#include "base/DirectXManager.h"

class CameraManager
{
private:
	static CameraManager* instance;
	static std::once_flag initInstanceFlag;

	CameraManager() = default;
	~CameraManager() = default;
	CameraManager(CameraManager&) = default;
	CameraManager& operator=(CameraManager&) = default;
public:
	// シングルトンインスタンスの取得
	static CameraManager* GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager);
	// 終了
	void Finalize();
	// カメラを追加する
	void AddCamera(std::unique_ptr<Camera> camera);

	// アクティブなカメラを更新する
	void Update();

	// アクティブなカメラを設定する
	void SetActiveCamera(const std::string& cameraName, float transitionTime = 0.0f);

	// アクティブなカメラを取得する
	Camera* GetActiveCamera() const;

	// アクティブなカメラをシェーダーに送る
	void BindCameraToShader();
	// 今ある全てのカメラを消す
	void DeleteAllCamera();


	// 今カメラ切り替えをしているかどうかの判定
	bool IsTransition() const { return isTransitioning_; }
private:
	// 補間の更新
	void TransitionUpdate();
	void CreateCameraResource();

	// カメラ座標
	struct CameraForGPU {
		Vector3 worldPosition;
	};
private:
	// カメラを名前で管理
	std::unordered_map<std::string, std::unique_ptr<Camera>> cameras_;
	// 現在のアクティブカメラ名
	std::string activeCameraName_;
	// 切り替わる先のカメラ名
	std::string nextCameraName_;

	DirectXManager* dxManager_ = nullptr;

	//Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;
	uint32_t cameraHandle_ = 0;

	CameraForGPU* cameraData_ = nullptr;

	// 補間関連
	bool isTransitioning_ = false;
	float transitionTime_ = 0.0f;
	float transitionTimer_ = 0.0f;
	Vector3 startPos_;
	Vector3 endPos_;
	Vector3 startRot_;
	Vector3 endRot_;
};