#include "Camera.h"
#include "imgui.h"

Camera::Camera(std::string cameraName)
	: transform_({ {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} })
	, horizontalFOV_(0.45f)
	, aspectRatio_(float(WindowManager::kClientWidth) / float(WindowManager::kClientHeight))
	, nearClip_(0.1f)
	, farClip_(200.0f)
	, worldMatrix_(MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate))
	, viewMatrix_(Inverse(worldMatrix_))
	, projectionMatrix_(MakePerspectiveFovMatrix(horizontalFOV_, aspectRatio_, nearClip_, farClip_))
{
	name = cameraName;
	// 一番最初に更新だけしておく
	Update();
}

void Camera::Update()
{
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(horizontalFOV_, aspectRatio_, nearClip_, farClip_);
	worldViewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
}

void Camera::LookAt(const Vector3& target)
{
    // カメラの現在位置
    Vector3 eye = transform_.translate;

    // 前方向（Z+を前としている前提で）
    Vector3 forward = Normalize(target - eye);

    // 仮のUpベクトル（Y軸）
    Vector3 up = { 0.0f, 1.0f, 0.0f };

    // 正確な右方向を計算
    Vector3 right = Normalize(Cross(up, forward));

    // 再計算された正確な上方向
    up = Cross(forward, right);

    // 回転行列作成
    Matrix4x4 matRot = {
        right.x,   right.y,   right.z,   0.0f,
        up.x,      up.y,      up.z,      0.0f,
        forward.x, forward.y, forward.z, 0.0f,
        0.0f,      0.0f,      0.0f,      1.0f
    };

    // 行列からオイラー角へ変換（Y軸→X軸→Z軸回転などの順序に合わせる必要あり）
    transform_.rotate = Transform(transform_.rotate, matRot);
}

