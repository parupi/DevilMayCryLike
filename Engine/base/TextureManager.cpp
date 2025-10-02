#include "TextureManager.h"

TextureManager* TextureManager::instance = nullptr;
std::once_flag TextureManager::initInstanceFlag;

// ImGuiで0番を使うため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager* TextureManager::GetInstance()
{
	std::call_once(initInstanceFlag, []() {
		instance = new TextureManager();
		});
	return instance;
}

void TextureManager::Initialize(DirectXManager* dxManager, SrvManager* srvManager)
{
	// SRVの数と同数
	textureData_.reserve(SrvManager::kMaxCount);

	dxManager_ = dxManager;
	srvManager_ = srvManager;
}

void TextureManager::Finalize()
{
	// テクスチャデータをクリア（ComPtrのリファレンスカウントが減り解放される）
	textureData_.clear();

	// 管理しているDirectXManagerとSrvManagerのポインタをクリア（所有権はない想定）
	dxManager_ = nullptr;
	srvManager_ = nullptr;

	// 必要なら、whiteTextureIndex_もリセット（任意）
	whiteTextureIndex_ = 0;

	delete instance;
	instance = nullptr;
}

void TextureManager::LoadTexture(const std::string& fileName)
{
	const std::string filePath = "Resource/Images/" + fileName;

	if (textureData_.contains(filePath)) {
		// 読み込み済みなら早期return
		return;
	}

	// テクスチャ枚数上限チェック
	ASSERT_MSG(srvManager_->CanAllocate(), "[TextureManager] Maximum number of textures reached. Cannot register new texture.");

	// テクスチャファイルを呼んでプログラムで扱えるようにする
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	// DDSファイルかどうか識別して適したほうで読み込み
	HRESULT hr = 0;
	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}

	ASSERT_MSG(SUCCEEDED(hr), ("[TextureManager] Failed to find texture"));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	// 圧縮フォーマットかどうか調べる
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		// 圧縮フォーマットならそのままmoveして使う
		mipImages = std::move(image);
	} else {
		hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
	}
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureData_[filePath] = TextureData();
	// 追加したテクスチャデータの参照を取得する
	TextureData& textureData = textureData_[filePath];

	textureData.srvIndex = srvManager_->Allocate();
	textureData.metadata = mipImages.GetMetadata();
	textureData.resource = dxManager_->CreateTextureResource(textureData.metadata);
	auto intermediate = dxManager_->UploadTextureData(textureData.resource.Get(), mipImages, dxManager_->GetDevice(), dxManager_->GetCommandList());
	dxManager_->FlushUpload();

	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	// SRVの設定を行う
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	// metaDataからCCubeMapかどうかを取得して分岐
	if (textureData.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(textureData.metadata.mipLevels);
	}

	 //設定をもとにSRVの生成
	dxManager_->GetDevice()->CreateShaderResourceView(textureData.resource.Get(), &srvDesc, textureData.srvHandleCPU);
}

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& fileName)
{
	const std::string filePath = "Resource/Images/" + fileName;

	if (textureData_.contains(filePath)) {
		// 読み込み済みなら要素番号を返す
		uint32_t textureIndex = textureData_.at(filePath).srvIndex;
		return textureIndex;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& fileName)
{
	assert(srvManager_->CanAllocate());
	const std::string filePath = "Resource/Images/" + fileName;

	TextureData& textureData = textureData_[filePath];
	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& fileName)
{
	const std::string filePath = "Resource/Images/" + fileName;

	// 範囲外指定違反チェック
	assert(srvManager_->CanAllocate());

	TextureData& textureData = textureData_[filePath];
	return textureData.metadata;
}

uint32_t TextureManager::CreateWhiteTexture()
{
	const std::string key = "__WHITE__";

	if (textureData_.contains(key)) {
		return textureData_[key].srvIndex;
	}

	// 1x1 白画像データ生成 (RGBA: 255,255,255,255)
	DirectX::TexMetadata metadata{};
	metadata.width = 1;
	metadata.height = 1;
	metadata.arraySize = 1;
	metadata.mipLevels = 1;
	metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

	DirectX::ScratchImage image{};

	// ✅ ピクセルデータありで初期化
	HRESULT hr = image.Initialize2D(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		1, // width
		1, // height
		1, // arraySize
		1  // mipLevels
	);
	assert(SUCCEEDED(hr));

	// ✅ ピクセルデータを書き込み
	uint8_t* pixels = image.GetPixels();
	pixels[0] = 255; // R
	pixels[1] = 255; // G
	pixels[2] = 255; // B
	pixels[3] = 255; // A

	// SRV確保
	TextureData texData{};
	texData.srvIndex = srvManager_->Allocate();
	texData.metadata = metadata;
	texData.resource = dxManager_->CreateTextureResource(texData.metadata);

	auto intermediate = dxManager_->UploadTextureData(texData.resource.Get(), image, dxManager_->GetDevice(), dxManager_->GetCommandList());
	dxManager_->FlushUpload();

	texData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(texData.srvIndex);
	texData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(texData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = metadata.format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = static_cast<UINT>(metadata.mipLevels);
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	dxManager_->GetDevice()->CreateShaderResourceView(texData.resource.Get(), &srvDesc, texData.srvHandleCPU);

	textureData_[key] = texData;
	whiteTextureIndex_ = texData.srvIndex;

	return texData.srvIndex;
}
