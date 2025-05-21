#pragma once
#include <vector>
#include <memory>
#include "Camera.h"
#include <mutex>

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
    void Initialize();
    // 終了
    void Finalize();
    // カメラを追加する
    void AddCamera(std::shared_ptr<Camera> camera);

    // アクティブなカメラを更新する
    void Update();

    // アクティブなカメラを設定する
    void SetActiveCamera(int index);

    // アクティブなカメラを取得する
    std::shared_ptr<Camera> GetActiveCamera() const;

    // アクティブなカメラをシェーダーに送る
    void BindCameraToShader();

private:
    std::vector<std::shared_ptr<Camera>> cameras_; // 管理するカメラのリスト
    int activeCameraIndex_; // アクティブなカメラのインデックス
};