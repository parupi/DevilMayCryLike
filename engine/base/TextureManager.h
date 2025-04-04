#pragma once
#include "DirectXManager.h"
#include "DirectXTex.h"
#include <d3d12.h>
#include <SrvManager.h>
#include <string>
#include <unordered_map>
#include <wrl.h>
#include <mutex>
class TextureManager
{
private:
	static TextureManager* instance;
	static std::once_flag initInstanceFlag;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = default;
	TextureManager& operator=(TextureManager&) = default;
public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	// 初期化
	void Initialize(DirectXManager* dxManager, SrvManager* srvManager);
	// 終了
	void Finalize();
public:
	void LoadTexture(const std::string& filePath);

	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	// メタデータを取得
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);
private:
	// テクスチャ1枚分のデータ
	struct TextureData {
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	// テクスチャデータ
	std::unordered_map<std::string, TextureData> textureData_;

	DirectXManager* dxManager_ = nullptr;
	SrvManager* srvManager_ = nullptr;
public:
	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

};