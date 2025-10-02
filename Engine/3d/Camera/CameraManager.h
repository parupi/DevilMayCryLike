#pragma once
#include <vector>
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
    void SetActiveCamera(int index);

    // アクティブなカメラを取得する
    Camera* GetActiveCamera() const;

    // アクティブなカメラをシェーダーに送る
    void BindCameraToShader();

    void DeleteAllCamera();
private:
    void CreateCameraResource();

    // カメラ座標
    struct CameraForGPU {
        Vector3 worldPosition;
    };
private:
    std::vector<std::unique_ptr<Camera>> cameras_; // 管理するカメラのリスト
    int activeCameraIndex_; // アクティブなカメラのインデックス
    DirectXManager* dxManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;

    CameraForGPU* cameraData_ = nullptr;
};