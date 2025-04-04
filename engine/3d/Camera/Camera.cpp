#include "Camera.h"
#include "imgui.h"

Camera::Camera()
	: transform_({ {1.0f,1.0f,1.0f},{0.3f,0.0f,0.0f},{0.0f,4.0f,-10.0f} })
	, horizontalFOV_(0.45f)
	, aspectRatio_(float(WindowManager::kClientWidth) / float(WindowManager::kClientHeight))
	, nearClip_(0.1f)
	, farClip_(200.0f)
	, worldMatrix_(MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate))
	, viewMatrix_(Inverse(worldMatrix_))
	, projectionMatrix_(MakePerspectiveFovMatrix(horizontalFOV_, aspectRatio_, nearClip_, farClip_))
{
	
}

void Camera::Update()
{
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(horizontalFOV_, aspectRatio_, nearClip_, farClip_);
	worldViewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
}

void Camera::FollowCamera(const Vector3& target)
{
	// カメラの位置を対象の後方に設定
	ImGui::Begin("Camera Manager");
	ImGui::DragFloat3("normalPos", &followCameraOffsetPosition_.x, 0.01f);
	ImGui::DragFloat3("normalRotate", &followCameraOffsetPosition_.x, 0.01f);
	ImGui::End();
	transform_.rotate = followCameraOffsetRotare_;
	transform_.translate = target + followCameraOffsetPosition_; // 例: 後方10ユニット、上に4ユニットオフセット
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);

}
