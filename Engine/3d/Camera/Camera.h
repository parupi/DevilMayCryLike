#pragma once
#include <math/function.h>
#include <base/WindowManager.h>
#include <memory>
#include <math/Matrix4x4.h>
#include <math/Vector3.h>
#include <math/Quaternion.h>
#include <math/Vector2.h>
class Camera
{
public: // メンバ関数
	Camera(std::string objectName);
	virtual ~Camera() = default;

	// 更新
	virtual void Update();

	// フォローカメラ
	void LookAt(const Vector3& target);
	// ワールド座標をスクリーン座標に変換
	Vector2 WorldToScreen(const Vector3& worldPos, int screenWidth, int screenHeight) const;

protected:
	EulerTransform transform_;
	Matrix4x4 worldMatrix_;
	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;
	Matrix4x4 worldViewProjectionMatrix_;
	// 水平方向視野角
	float horizontalFOV_;
	// アスペクト比
	float aspectRatio_;
	// ニアクリップ距離
	float nearClip_;
	// ファークリップ距離
	float farClip_;

	Vector3 followCameraOffsetPosition_ = Vector3(0.0f, 30.0f, -50.0f);

	Vector3 followCameraOffsetRotate_ = Vector3(0.51f, 0.0f, 0.0f);



public: // ゲッター // セッター //
	// 回転
	Vector3& GetRotate() { return transform_.rotate; }
	// 拡縮
	Vector3& GetTranslate() { return transform_.translate; }

	// カメラの前方向（Z+が前提）
	Vector3 GetForward() const;

	// カメラの右方向（X+が右）
	Vector3 GetRight() const;


	void SetFovY(const float& horizontalFOV) { horizontalFOV_ = horizontalFOV; }
	void SetAspectRate(const float& aspectRatio) { aspectRatio_ = aspectRatio; }
	void SetNearClip(const float& nearClip) { nearClip_ = nearClip; }
	void SetFarClip(const float& farClip) { farClip_ = farClip; }

	const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
	const Matrix4x4& GetViewProjectionMatrix() const { return worldViewProjectionMatrix_; }

	std::string name_;
};

