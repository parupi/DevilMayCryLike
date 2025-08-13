#include "Camera.h"
#include "imgui/imgui.h"
#include "math/function.h"
#include <math/Vector4.h>

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
	name_ = cameraName;
	// 一番最初に更新だけしておく
	Update();
}

void Camera::Update()
{
#ifdef _DEBUG
	ImGui::Begin("camera");
	ImGui::DragFloat3("translate0", &transform_.translate.x, 0.01f);
	ImGui::DragFloat3("rotate0", &transform_.rotate.x, 0.01f);
	ImGui::End();
#endif // _DEBUG

	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	viewMatrix_ = Inverse(worldMatrix_);
	projectionMatrix_ = MakePerspectiveFovMatrix(horizontalFOV_, aspectRatio_, nearClip_, farClip_);
	worldViewProjectionMatrix_ = viewMatrix_ * projectionMatrix_;
}

void Camera::LookAt(const Vector3& target) {
	Vector3 eye = transform_.translate;
	Vector3 forward = Normalize(target - eye);

	// 水平方向のみの回転（XZ平面に投影）
	forward.y = 0.0f;
	forward = Normalize(forward);

	// Y軸の回転角だけ計算
	float angle = std::atan2(forward.x, forward.z); // Y軸回転角（ラジアン）

	transform_.rotate = { transform_.rotate.x, angle, transform_.rotate.z };
}

Vector2 Camera::WorldToScreen(const Vector3& worldPos, int screenWidth, int screenHeight) const
{
	// 1. ワールド座標をビュー空間に変換
	Vector4 clipPos = Vector4(worldPos.x, worldPos.y, worldPos.z, 1.0f) * viewMatrix_ * projectionMatrix_;

	// 2. NDCに変換 (透視投影のため、wで割る)
	if (clipPos.w != 0.0f) {
		clipPos.x /= clipPos.w;
		clipPos.y /= clipPos.w;
		clipPos.z /= clipPos.w;
	}

	// 3. スクリーン座標に変換
	Vector2 screenPos;
	screenPos.x = (clipPos.x * 0.5f + 0.5f) * screenWidth;
	screenPos.y = (1.0f - (clipPos.y * 0.5f + 0.5f)) * screenHeight; // Y軸を反転

	return screenPos;
}

// カメラの前方向を取得
Vector3 Camera::GetForward() const {
	Matrix4x4 rotMat = MakeRotateXYZMatrix(transform_.rotate);
	// 第3列がZ方向（前方向） ※左手系Z+前提
	return Normalize(Vector3{
		rotMat.m[2][0],
		rotMat.m[2][1],
		rotMat.m[2][2]
		});
}

// カメラの右方向を取得
Vector3 Camera::GetRight() const {
	Matrix4x4 rotMat = MakeRotateXYZMatrix(transform_.rotate);
	// 第1列がX方向（右方向）
	return Normalize(Vector3{
		rotMat.m[0][0],
		rotMat.m[0][1],
		rotMat.m[0][2]
		});
}
