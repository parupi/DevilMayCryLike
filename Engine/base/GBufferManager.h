#pragma once
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include <memory>
#include <mutex>

class DirectXManager;

class GBufferManager
{
private:
	static GBufferManager* instance;
	static std::once_flag initInstanceFlag;

	GBufferManager() = default;
	~GBufferManager() = default;
	GBufferManager(const GBufferManager&) = default;
	GBufferManager& operator=(const GBufferManager&) = default;
public:
	// シングルトンインスタンスの取得
	static GBufferManager* GetInstance();

	enum class GBufferType
	{
		Albedo,
		Normal,
		Depth,    // DepthはDSV兼SRV化できるようにする
		Count
	};

	void Initialize(DirectXManager* dxManager);

	void Finalize();

	// RTV / SRV / DSV取得用
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle(GBufferType type) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const; // 深度用
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandle(GBufferType type) const;

	ID3D12Resource* GetResource(GBufferType type) const;

private:
	void CreateResources(UINT width, UINT height);
	void CreateRTVs();
	void CreateSRVs();
	void CreateDSV();

private:
	DirectXManager* dxManager_ = nullptr;

	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, (size_t)GBufferType::Count> gBufferResources_;

	// RTV Heap / SRV Heap / DSV Heap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;

	UINT rtvIncrement_ = 0;
	UINT srvIncrement_ = 0;
};

