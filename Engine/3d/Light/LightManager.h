#pragma once
#include <memory>
#include <math/Vector4.h>
#include <math/Vector3.h>
#include <wrl.h>
#include <d3d12.h>
#include <mutex>
#include "BaseLight.h"
#include "LightStructs.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include <base/DirectXManager.h>
#include <vector>

class LightManager
{
private:
	static LightManager* instance;
	static std::once_flag initInstanceFlag;

	LightManager() = default;
	~LightManager() = default;
	LightManager(LightManager&) = default;
	LightManager& operator=(LightManager&) = default;
public:
	// シングルトンインスタンスの取得
	static LightManager* GetInstance();
	// 初期化処理
	void Initialize(DirectXManager* dxManager);
	// 終了処理
	void Finalize();
	// 更新処理
	void Update();
	// ライトの追加
	void AddLight(std::unique_ptr<BaseLight> light);
	// 全ライトの削除
	void DeleteAllLight();
	// GPU 転送
	void UploadToGPU();
	// DescriptorHeap に SRV を作成（構造化バッファ版）
	void CreateLightBuffer();
	// ライトデータをGPUにバインド
	void BindLightsToShader();
private:
	DirectXManager* dxManager_ = nullptr;

	// すべてのライト
	std::vector<std::unique_ptr<BaseLight>> lights_;

	// GPU 用のバッファ（StructuredBuffer）
	Microsoft::WRL::ComPtr<ID3D12Resource> lightBuffer_;   // LightData 配列
	Microsoft::WRL::ComPtr<ID3D12Resource> lightCountBuffer_; // ライト数（定数バッファ用）

	// CPU-side temporary buffer
	std::vector<LightData> gpuLightCache_;

	// ライトの最大数
	const UINT MaxLights = 256;
	// srvIndex
	uint32_t srvIndex_ = 0;
};

