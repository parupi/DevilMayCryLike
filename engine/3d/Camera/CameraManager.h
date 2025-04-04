#pragma once
#include <vector>
#include <memory>
#include "Camera.h"

class CameraManager
{
public:
    CameraManager();
    ~CameraManager();

    // カメラを追加する
    void AddCamera(std::shared_ptr<Camera> camera);

    // アクティブなカメラを更新する
    void Update();

    // アクティブなカメラを設定する
    void SetActiveCamera(int index);

    // アクティブなカメラを取得する
    std::shared_ptr<Camera> GetActiveCamera() const;

private:
    std::vector<std::shared_ptr<Camera>> cameras_; // 管理するカメラのリスト
    int activeCameraIndex_; // アクティブなカメラのインデックス
};