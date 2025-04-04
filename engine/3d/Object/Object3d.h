#pragma once
#include "Object3dManager.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "function.h"
#include <fstream>
#include "Model.h"
#include <Camera.h>
class Object3dManager;
class WorldTransform;

class Object3d
{
public: // メンバ関数
	Object3d() = default;
	~Object3d();

	void Initialize(const std::string& filePath);
	// アニメーション用アップデート
	void AnimationUpdate();
	void Update();
	void Draw();

#ifdef _DEBUG
	void DebugGui();
#endif // _DEBUG


private:
	void CreateMaterialResource();
	void CreateCameraResource();
private: // 構造体
	struct Material {
		Vector4 color;
		bool enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
		float shininess;
	};

	// カメラ座標
	struct CameraForGPU {
		Vector3 worldPosition;
	};

private: // メンバ変数
	Object3dManager* objectManager_ = nullptr;
	Model* model_ = nullptr;
	Camera* camera_ = nullptr;
	//Animator* animator_ = nullptr;
	
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_ = nullptr;

	
	Material* materialData_ = nullptr;
	CameraForGPU* cameraData_ = nullptr;

	EulerTransform uvTransform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Vector2 uvPosition_ = { 0.0f, 0.0f };
	float uvRotation_ = 0.0f;
	Vector2 uvSize_ = { 1.0f, 1.0f };

	std::unique_ptr<WorldTransform> transform_;
public: // ゲッター // セッター // 
	// モデル
	void SetModel(Model* model) { model_ = model; }
	void SetModel(const std::string& filePath);
	Model* GetModel() { return model_; }
	//Animator* GetAnimator() { return animator_; }
	// カメラ
	void SetCamera(Camera* camera) { camera_ = camera; }
	// 色
	const Vector4& GetColor() const { return materialData_->color; }
	void SetColor(const Vector4& color) { materialData_->color = color; }
	// Lighting
	const bool& GetIsLighting() const { return materialData_->enableLighting; }
	void SetIsLighting(const bool isLighting) { materialData_->enableLighting = isLighting; }
	// uv平行移動
	const Vector2& GetUVPosition() const { return uvPosition_; }
	void SetUVPosition(const Vector2& position) { uvPosition_ = position; }
	// 回転
	float GetUVRotation() const { return uvRotation_; }
	void SetUVRotation(float rotation) { uvRotation_ = rotation; }
	// 拡縮
	const Vector2& GetUVSize() const { return uvSize_; }
	void SetUVSize(const Vector2& size) { uvSize_ = size; }
	// ワールドトランスフォームの取得
	WorldTransform* GetWorldTransform() { return transform_.get(); }
};