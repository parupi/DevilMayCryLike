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
#include "Graphics/Device/DirectXManager.h"
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
	static LightManager* GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	void Update();
	void AddLight(std::unique_ptr<BaseLight> light);
	void DeleteAllLight();

	void BindLightsToShader();
	// 全ライトの情報を取得
	std::vector<LightData> GetAllLightData() { return gpuLightCache_; }
private:
	// 各バッファの生成
	void CreateLightBuffers();

	DirectXManager* dxManager_ = nullptr;

	uint32_t lightBufferHandle_ = 0;
	uint32_t lightCountHandle_ = 0;

	// 永続 Map されたポインタ
	LightData* mappedLightPtr_ = nullptr;
	UINT* mappedCountPtr_ = nullptr;

	// すべてのライト
	std::vector<std::unique_ptr<BaseLight>> lights_;
	std::vector<LightData> gpuLightCache_;

	// ライトの最大数
	const UINT MaxLights = 128;
	// srvIndex
	uint32_t srvIndex_ = 0;
};

