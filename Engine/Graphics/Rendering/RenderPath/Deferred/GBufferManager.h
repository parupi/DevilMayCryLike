#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include <memory>

class DirectXManager;

class GBufferManager
{
public:

	enum class GBufferType
	{
		Albedo,
		Normal,
		WorldPos,
		Material,
		//Depth,
		Count
	};

	void Initialize(DirectXManager* dxManager);

	void Finalize();

	void TransitionAllToReadable();
	void TransitionAllToRT();

	// RTV / SRV / DSV取得用
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(GBufferType type) const;
	//D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const; // 深度用
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle(GBufferType type) const;

	//uint32_t GetDepthIndex() const { return srvIndices_[(UINT)GBufferType::Depth]; }

	ID3D12Resource* GetResource(GBufferType type) const;

private:
	void CreateResources(UINT width, UINT height);
	void CreateRTVs();
	void CreateSRVs();
	void CreateDSV();

private:
	DirectXManager* dxManager_ = nullptr;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, (size_t)GBufferType::Count> gBufferResources_;
	std::array<UINT, (size_t)GBufferType::Count> rtvIndices_;
	std::array<UINT, (size_t)GBufferType::Count> srvIndices_;
	UINT dsvIndex_;
};

