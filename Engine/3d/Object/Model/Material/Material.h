#pragma once
#include <3d/Object/Model/ModelStructs.h>
#include "Graphics/Device/DirectXManager.h"
#include "Graphics/Resource/SrvManager.h"

class SrvManager;
class DirectXManager;

class Material
{
public:
	Material();
	~Material();
	// 初期化
	void Initialize(DirectXManager* directXManager, SrvManager* srvManager, MaterialData materialData);
	// 更新処理
	void Update(const Vector3& objectScale = {1.0f, 1.0f, 1.0f});

	// 描画
	void Bind(UINT RootParameterIndex);
	// GBufferへの描画
	void BindForGBuffer();

#ifdef _DEBUG
	void DebugGui(uint32_t index);
#endif // _DEBUG

	// TextureDensityの維持をするかどうかのフラグを設定
	void SetEnableTextureDensity(bool enable) { enableTextureDensity_ = enable; }
private:
	void CreateMaterialResource();
	void CreateGBufferMaterialResource();

private:
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialGBufferResource_ = nullptr;

	uint32_t materialHandle_ = kInvalidBufferHandle;
	uint32_t materialGBufferHandle_ = kInvalidBufferHandle;

	MaterialForGPU* materialForGPU_ = nullptr;
	GBufferMaterialParam* gBufferMaterialParam_ = nullptr;
	MaterialData materialData_;

	DirectXManager* directXManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	EulerTransform uvTransform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	UVData uvData_;

	// TextureDensityの維持をするかどうかのフラグ
	bool enableTextureDensity_ = false;
	// テクスチャ密度を維持するためのスケール値
	float textureDensityScale_ = 1.0f;

public:
	// 色
	Vector4& GetColor() const { return materialForGPU_->color; }
	void SetColor(const Vector4& color) { materialForGPU_->color = color; }
	// Lighting
	bool GetIsLighting() const { return materialForGPU_->enableLighting; }
	void SetIsLighting(const bool isLighting) { materialForGPU_->enableLighting = isLighting; }

	void SetEnvironmentIntensity(float intensity) {materialForGPU_->environmentIntensity = intensity; }

	UVData& GetUVData() { return uvData_; }
};

