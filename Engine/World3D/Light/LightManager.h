#pragma once
#include <memory>
#include <Math/Vector4.h>
#include <Math/Vector3.h>
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
#include "Graphics/Rendering/Shadow/CascadedShadowMap.h"

class LightManager
{
private:
	LightManager() = default;
	LightManager(const LightManager&) = delete;
	LightManager& operator=(const LightManager&) = delete;
public:
	static LightManager& GetInstance();

	void Initialize(DirectXManager* dxManager);
	void Finalize();

	void Update();
	// ライトを追加し、追加したライトへのポインタを返す（所有権はLightManager）
	BaseLight* AddLight(std::unique_ptr<BaseLight> light);
	// 指定したライトだけを削除する（見つからなければ何もしない）
	void RemoveLight(BaseLight* light);
	void DeleteAllLight();

	void BindLightsToShader();
	// 全ライトの情報を取得
	std::vector<LightData> GetAllLightData() { return gpuLightCache_; }

	CascadedShadowMap* GetCSM() { return csm.get(); }

#ifdef _DEBUG
	// デバッグ描画をする
	void DrawDebug();
#endif
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

	std::unique_ptr<CascadedShadowMap> csm = nullptr;

	// デバッグ用
#ifdef _DEBUG
	// エディターの描画
	void DrawLightEditor();
	int32_t selectedLightIndex_ = 0;
#endif
};

