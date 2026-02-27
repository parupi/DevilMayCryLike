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
	void Update();

	// 描画
	void Bind(UINT RootParameterIndex);
	// GBufferへの描画
	void BindForGBuffer();

#ifdef _DEBUG
	void DebugGui(uint32_t index);
#endif // _DEBUG

private:
	void CreateMaterialResource();
	void CreateGBufferMaterialResource();

private:
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_ = nullptr;
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialGBufferResource_ = nullptr;

	uint32_t materialHandle_ = 0;
	uint32_t materialGBufferHandle_ = 0;

	MaterialForGPU* materialForGPU_ = nullptr;
	GBufferMaterialParam* gBufferMaterialParam_ = nullptr;
	MaterialData materialData_;

	DirectXManager* directXManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	EulerTransform uvTransform_{ {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	UVData uvData_;

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

