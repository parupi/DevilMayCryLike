#pragma once
#include <vector>
#include <Vector4.h>
#include <Vector3.h>
#include <wrl.h>
#include <d3d12.h>
#include <DirectXManager.h>

constexpr int maxDirLights = 3;
constexpr int maxPointLights = 3; // ポイントライトの最大数
constexpr int maxSpotLights = 3;   // スポットライトの最大数

struct DirectionalLight {
	Vector4 color;		//!< ライトの色
	Vector3 direction;	//!< ライトの向き
	float intensity;	//!< 輝度
	int enabled;
};

// pointLight
struct PointLight {
	Vector4 color; //!<ライトの色
	Vector3 position; //!<ライトの位置
	float intensity; //!< 輝度
	float radius; //!< ライトの届く最大距離
	float decay; //!< 減衰率
	int enabled;
	float padding[2];
};

// spotLight
struct SpotLight {
	Vector4 color; //!< ライトの色
	Vector3 position; //!< ライトの位置
	float intensity; //!< 輝度
	Vector3 direction; //!< ライトの向き
	float distance; //!< ライトの届く最大距離
	float decay; //!< 減衰率
	float cosAngle; //!< スポットライトの余弦
	int enabled;
	float padding[2];
};

class LightManager
{
public:
	void Initialize();
	void BindLightsToShader();
private:
	void CreateDirLightResource();
	void CreatePointLightResource();
	void CreateSpotLightResource();
private:
	DirectXManager* dxManager_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> dirLightResource1_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> dirLightResource2_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> dirLightResource3_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource1_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource2_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource3_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource1_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource2_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource3_ = nullptr;

	DirectionalLight* dirLightData1_ = nullptr;
	DirectionalLight* dirLightData2_ = nullptr;
	DirectionalLight* dirLightData3_ = nullptr;
	PointLight* pointLightData1_ = nullptr;
	PointLight* pointLightData2_ = nullptr;
	PointLight* pointLightData3_ = nullptr;
	SpotLight* spotLightData1_ = nullptr;
	SpotLight* spotLightData2_ = nullptr;
	SpotLight* spotLightData3_ = nullptr;

public: // ゲッター // セッター //
	// DirLight
	Vector4 GetDirLightColor(uint32_t index) const;
	void SetDirLightColor(uint32_t index, const Vector4& color);
	Vector3 GetDirLightDirection(uint32_t index) const;
	void SetDirLightDirection(uint32_t index, const Vector3& dir);
	float GetDirLightIntensity(uint32_t index) const;
	void SetDirLightIntensity(uint32_t index, float intensity);
	bool GetDirLightActive(uint32_t index) const;
	void SetDirLightActive(uint32_t index, bool isActive);
	// PointLight
	Vector4 GetPointLightColor(uint32_t index) const;
	void SetPointLightColor(uint32_t index, const Vector4& color);
	Vector3 GetPointLightPos(uint32_t index) const;
	void SetPointLightPos(uint32_t index, const Vector3& pos);
	float GetPointLightIntensity(uint32_t index) const;
	void SetPointLightIntensity(uint32_t index, float intensity);
	float GetPointLightRadius(uint32_t index) const;
	void SetPointLightRadius(uint32_t index, float radius);
	void SetPointLightDecay(uint32_t index, float decay);
	float GetPointLightDecay(uint32_t index) const;
	bool GetPointLightActive(uint32_t index) const;
	void SetPointLightActive(uint32_t index, bool isActive);
	// SpotLight
	void SetSpotLightColor(uint32_t index, const Vector4& color);
	void SetSpotLightPos(uint32_t index, const Vector3& pos);
	void SetSpotLightIntensity(uint32_t index, float intensity);
	void SetSpotLightDirection(uint32_t index, const Vector3& dir);
	void SetSpotLightDistance(uint32_t index, float distance);
	void SetSpotLightDecay(uint32_t index, float decay);
	void SetSpotLightCosAngle(uint32_t index, float cosAngle);
	void SetSpotLightActive(uint32_t index, bool isActive);
	Vector4 GetSpotLightColor(uint32_t index) const;
	Vector3 GetSpotLightPos(uint32_t index) const;
	float GetSpotLightIntensity(uint32_t index) const;
	Vector3 GetSpotLightDirection(uint32_t index) const;
	float GetSpotLightDistance(uint32_t index) const;
	float GetSpotLightDecay(uint32_t index) const;
	float GetSpotLightCosAngle(uint32_t index) const;
	bool GetSpotLightActive(uint32_t index) const;
};

