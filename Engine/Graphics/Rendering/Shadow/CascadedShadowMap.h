#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include "ShadowTypes.h"
#include "World3D/Light/LightStructs.h"

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
	// バインド（シャドウパス用: 各カスケードの VP 行列をセット）
	void Bind(uint32_t rootParameterIndex, uint32_t cascadeIndex);
	// カスケード開始
	void BeginCascade(uint32_t index);
	// カスケード終了
	void EndCascade(uint32_t index);
	// シャドウマップ SRV をバインド（ライティングパス用）
	void BindSrv();
	// カスケード定数バッファをバインド（ライティングパス用）
	void BindCascadeCB(uint32_t rootParameterIndex);

	// カスケードの情報を取得
	const CascadeData* GetCascadeData() const { return cascades_.data(); }
	// シャドウマップサイズを取得
	uint32_t GetShadowMapSize() const { return shadowMapSize_; }

#ifdef _DEBUG
	void DrawDebugUI();
#endif

private:
	void CalculateCascadeSplits();
	void CalculateLightMatrices();
	void GetFrustumCornersViewSpace(float fovY, float aspect, float nearZ, float farZ, Vector3 outCorners[8]);
	void CreateDSV();

	struct LightVPConstants
	{
		Matrix4x4 lightViewProj;
	};

	// ライティングパスに渡す全カスケードデータ（HLSL の LightMatrixCB と一致させる）
	struct CascadeLightingCB
	{
		Matrix4x4 lightViewProj0;   // cascade 0 の Light VP 行列
		Matrix4x4 lightViewProj1;   // cascade 1 の Light VP 行列
		Matrix4x4 lightViewProj2;   // cascade 2 の Light VP 行列
		float     cascadeFar[4];    // xyz = 各カスケードの view-space far 深度
		Matrix4x4 cameraView;       // カメラビュー行列（view-space Z 計算用）
	};

private:
	DirectXManager* dxManager_ = nullptr;
	BaseCamera* camera_ = nullptr;
	std::vector<LightData> lights_;

	uint32_t shadowMapSize_ = 1280;

	// 調整可能パラメータ
	float shadowDistance_ = 100.0f;  // フラスタム中心からライトを離す距離（影の送り主カバー用）
	float shadowFar_      = 300.0f;  // 最大シャドウ距離（全カスケードのfar平面）
	float splitLambda_    = 0.7f;    // カスケード分割: 対数(1.0)と均等(0.0)のブレンド係数

	std::array<CascadeData, kCascadeCount> cascades_;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kCascadeCount> shadowMaps_;
	//std::array<D3D12_CPU_DESCRIPTOR_HANDLE, kCascadeCount> dsvHandles_;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kCascadeCount> lightVPBuffers_;
	std::array<uint32_t, kCascadeCount> lightVPIndex_;
	std::array<LightVPConstants*, kCascadeCount> mappedLightVP_;

	uint32_t           lightingCBIndex_ = 0;
	CascadeLightingCB* mappedLightingCB_ = nullptr;

	std::array<uint32_t, kCascadeCount> dsvIndices_;
	std::array<uint32_t, kCascadeCount> srvIndices_;
};