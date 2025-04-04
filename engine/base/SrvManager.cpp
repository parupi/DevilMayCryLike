#include "SrvManager.h"

const uint32_t SrvManager::kMaxCount = 512;

void SrvManager::Initialize(DirectXManager* dxManager)
{
	dxManager_ = dxManager;
	// デスクリプタヒープの生成
	descriptorHeap_ = dxManager_->CreateDescriptorHeap(dxManager_->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, kMaxCount, true);
	// デスクリプタ1個分のサイズを取得して記録
	descriptorSize_ = dxManager_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

uint32_t SrvManager::Allocate()
{
	assert(useIndex < kMaxCount);

	// returnする番号を一旦記録しておく
	int index = useIndex;
	// 次回のために番号を1進める
	useIndex++;
	// 上で記録した番号を返す
	return index;
}

void SrvManager::BeginDraw()
{
	// SRV用のデスクリプタヒープを指定する
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeaps[] = { descriptorHeap_ };
	dxManager_->GetCommandList()->SetDescriptorHeaps(1, descriptorHeaps->GetAddressOf());
}

bool SrvManager::CanAllocate() const
{
	return useIndex < kMaxCount;
}

D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index)
{
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap_->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize_ * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index)
{
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap_->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize_ * index);
	return handleGPU;
}

void SrvManager::CreateSRVforTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT MipLevels)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// テクスチャ2D用のSRV設定
	srvDesc.Format = format; // 指定されたフォーマットを使用
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャとしてビューを作成
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // デフォルトのシェーダマッピング
	srvDesc.Texture2D.MostDetailedMip = 0; // 最も詳細なミップマップのレベル
	srvDesc.Texture2D.MipLevels = MipLevels; // ミップレベルの数
	srvDesc.Texture2D.PlaneSlice = 0; // 通常、0に設定
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f; // 最小のレベルの詳細度をクランプ

	dxManager_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::CreateSRVforStructuredBuffer(uint32_t srvIndex, ID3D12Resource* pResource, UINT numElements, UINT structureByteStride)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	// 構造化バッファ用のSRV設定
	srvDesc.Format = DXGI_FORMAT_UNKNOWN; // 構造化バッファの場合、フォーマットは不明（UNKNOWN）
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER; // バッファとしてビューを作成
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // デフォルトのシェーダマッピング
	srvDesc.Buffer.FirstElement = 0; // バッファ内の最初の要素のインデックス
	srvDesc.Buffer.NumElements = numElements; // バッファ内の要素数
	srvDesc.Buffer.StructureByteStride = structureByteStride; // 構造体のバイト単位のサイズ
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE; // 特殊なフラグは無し

	dxManager_->GetDevice()->CreateShaderResourceView(pResource, &srvDesc, GetCPUDescriptorHandle(srvIndex));
}

void SrvManager::SetGraphicsRootDescriptorTable(UINT RootParameterIndex, uint32_t srvIndex)
{
	dxManager_->GetCommandList()->SetGraphicsRootDescriptorTable(RootParameterIndex, GetGPUDescriptorHandle(srvIndex));
}
