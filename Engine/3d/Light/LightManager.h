#pragma once
#include <vector>
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

static inline size_t Align256(size_t s);

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
	// 描画前処理
	void BindLightsToShader();
	// ライトの更新処理
	void UpdateAllLight();
	// ライトの追加
	// ライト作成
	DirectionalLight* CreateDirectionalLight(const std::string& name);
	PointLight* CreatePointLight(const std::string& name);
	SpotLight* CreateSpotLight(const std::string& name);
	void CreateDummyLightResources();

	// 取得
	DirectionalLight* GetDirectionalLight(const std::string& name);
	PointLight* GetPointLight(const std::string& name);
	SpotLight* GetSpotLight(const std::string& name);

	void DeleteAllLight();
private:
	DirectXManager* dxManager_ = nullptr;

	// 集約された GPU バッファ（各フレームの描画用）
	Microsoft::WRL::ComPtr<ID3D12Resource> aggregatedDirBuffer_;
	Microsoft::WRL::ComPtr<ID3D12Resource> aggregatedPointBuffer_;
	Microsoft::WRL::ComPtr<ID3D12Resource> aggregatedSpotBuffer_;

	std::vector<std::unique_ptr<DirectionalLight>> dirLights_;
	std::vector< std::unique_ptr<PointLight>> pointLights_;
	std::vector<std::unique_ptr<SpotLight>> spotLights_;

	// 各ライトのダミーリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> dummyDirLight_;
	Microsoft::WRL::ComPtr<ID3D12Resource> dummyPointLight_;
	Microsoft::WRL::ComPtr<ID3D12Resource> dummySpotLight_;

	// 内部ユーティリティ
	void CreateAggregatedBuffers();
	void UpdateAggregatedBuffers();
	void UpdateBuffer(ID3D12Resource* resource, const void* data, size_t size);
};

