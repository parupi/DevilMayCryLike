#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include "ShadowTypes.h"
#include "3d/Light/LightStructs.h"

class DirectXManager;
class BaseCamera;

class CascadedShadowMap
{
public:
	CascadedShadowMap() = default;
	~CascadedShadowMap() = default;

	// 初期化
	void Initialize(DirectXManager* dxManager, uint32_t shadowMapSize);
	// 更新処理
	void Update();
	// バインド
	void Bind(uint32_t rootParameterIndex, uint32_t cascadeIndex);
	// カスケード開始
	void BeginCascade(uint32_t index);
	// カスケード終了
	void EndCascade(uint32_t index);

	// カスケードの情報を取得
	const CascadeData* GetCascadeData() const { return cascades_.data(); }
	// シャドウマップサイズを取得
	uint32_t GetShadowMapSize() const { return shadowMapSize_; }

private:
	void CalculateCascadeSplits();
	void CalculateLightMatrices();
	void GetFrustumCornersViewSpace(float fovY, float aspect, float nearZ, float farZ, Vector3 outCorners[8]);
	void CreateDSV();

	struct LightVPConstants
	{
		Matrix4x4 lightViewProj;
	};

private:
	DirectXManager* dxManager_ = nullptr;
	BaseCamera* camera_ = nullptr;
	std::vector<LightData> lights_;

	uint32_t shadowMapSize_ = 1280;

	std::array<CascadeData, kCascadeCount> cascades_;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kCascadeCount> shadowMaps_;
	//std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kCascadeCount> dsvHandles_;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kCascadeCount> lightVPBuffers_;
	std::array<uint32_t, kCascadeCount> lightVPIndex_;
	std::array<LightVPConstants*, kCascadeCount> mappedLightVP_;

	std::array<uint32_t, kCascadeCount> dsvIndices_;
	std::array<uint32_t, kCascadeCount> srvIndices_;
};