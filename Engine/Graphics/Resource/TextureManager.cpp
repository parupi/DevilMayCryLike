#include "TextureManager.h"
#include <cmath>
#include <algorithm>


// ImGuiで0番を使うため、1番から使用
uint32_t TextureManager::kSRVIndexTop = 1;

TextureManager& TextureManager::GetInstance() {
	static TextureManager instance;
	return instance;
}

void TextureManager::Initialize(DirectXManager* dxManager) {
	// SRVの数と同数
	textureData_.reserve(SrvManager::kMaxCount);

	dxManager_ = dxManager;
	srvManager_ = dxManager_->GetSrvManager();

	CreateDissolveNoiseTexture();
	CreateRandomDissolveNoiseTexture();
}

void TextureManager::Finalize() {
	// テクスチャデータをクリア
	textureData_.clear();

	// 管理しているDirectXManagerとSrvManagerのポインタをクリア
	dxManager_ = nullptr;
	srvManager_ = nullptr;

	// 必要なら、whiteTextureIndex_もリセット（任意）
	whiteTextureIndex_ = 0;

}

void TextureManager::LoadTexture(const std::string& fileName) {
	const std::string filePath = "Resource/Images/" + fileName;

	// 既に読み込み済みなら終了
	if (textureData_.contains(filePath)) {
		return;
	}

	ASSERT_MSG(srvManager_->CanAllocate(),
		"[TextureManager] Maximum number of textures reached.");

	// ---------------------------
	// 1. 画像ファイル読み込み
	// ---------------------------
	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = 0;

	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	} else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	ASSERT_MSG(SUCCEEDED(hr), "[TextureManager] Failed to load texture file.");

	// ---------------------------
	// 2. ミップマップ生成
	// ---------------------------
	DirectX::ScratchImage mipImages;

	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	} else {
		const auto& meta = image.GetMetadata();
		if (meta.width == 1 && meta.height == 1) {
			mipImages = std::move(image);
		} else {
			hr = DirectX::GenerateMipMaps(
				image.GetImages(),
				image.GetImageCount(),
				meta,
				DirectX::TEX_FILTER_SRGB,
				0,
				mipImages
			);
			ASSERT_MSG(SUCCEEDED(hr), "[TextureManager] MipMap generation failed.");
		}
	}

	const auto& meta = mipImages.GetMetadata();

	// ---------------------------
	// 3. ResourceFactory による GPU リソース作成
	// ---------------------------
	TextureData tex{};
	tex.srvIndex = srvManager_->Allocate();
	tex.metadata = meta;

	// GPUテクスチャ生成（初期状態 COPY_DEST）
	tex.resource = dxManager_->GetResourceFactory()->CreateTexture2D(meta);

	// ---------------------------
	// 4. テクスチャデータアップロード
	// ---------------------------
	dxManager_->UploadTextureData(tex.resource.Get(), mipImages);

	// ---------------------------
	// 5. SRV 生成
	// ---------------------------
	tex.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(tex.srvIndex);
	tex.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(tex.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = meta.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (meta.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = static_cast<UINT>(meta.mipLevels);
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	} else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = static_cast<UINT>(meta.mipLevels);
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	}

	dxManager_->GetDevice()->CreateShaderResourceView(
		tex.resource.Get(),
		&srvDesc,
		tex.srvHandleCPU
	);

	// 最終登録
	textureData_[filePath] = std::move(tex);
}


uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& fileName) {
	const std::string filePath = "Resource/Images/" + fileName;

	if (textureData_.contains(filePath)) {
		// 読み込み済みなら要素番号を返す
		uint32_t textureIndex = textureData_.at(filePath).srvIndex;
		return textureIndex;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& fileName) {
	assert(srvManager_->CanAllocate());
	const std::string filePath = "Resource/Images/" + fileName;

	TextureData& textureData = textureData_[filePath];
	return textureData.srvHandleGPU;
}

const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& fileName) {
	const std::string filePath = "Resource/Images/" + fileName;

	// 範囲外指定違反チェック
	assert(srvManager_->CanAllocate());

	TextureData& textureData = textureData_[filePath];
	return textureData.metadata;
}

uint32_t TextureManager::CreateWhiteTexture() {
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

	// ピクセルデータありで初期化
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
	texData.resource = dxManager_->GetResourceFactory()->CreateTexture2D(metadata);

	auto intermediate = dxManager_->UploadTextureData(texData.resource.Get(), image);

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

// ---------------------------------------------------------------------------
// CPU側ノイズ実装（Dissolve.hlsli の Hash21/ValueNoise と同一アルゴリズム）
// ---------------------------------------------------------------------------
namespace {
	static float NoiseFrac(float v) { return v - std::floor(v); }

	static float NoiseHash21(float px, float py) {
		float qx = NoiseFrac(px * 127.1f);
		float qy = NoiseFrac(py * 311.7f);
		float d = qx * (qx + 45.32f) + qy * (qy + 45.32f);
		return NoiseFrac(NoiseFrac(qx + d) * NoiseFrac(qy + d));
	}

	static float NoiseValue(float ux, float uy) {
		float ix = std::floor(ux), iy = std::floor(uy);
		float fx = NoiseFrac(ux), fy = NoiseFrac(uy);
		float sx = fx * fx * (3.0f - 2.0f * fx);
		float sy = fy * fy * (3.0f - 2.0f * fy);
		float a = NoiseHash21(ix, iy);
		float b = NoiseHash21(ix + 1.f, iy);
		float c = NoiseHash21(ix, iy + 1.f);
		float d = NoiseHash21(ix + 1.f, iy + 1.f);
		return a + (b - a) * sx + (c - a) * sy + (a - b - c + d) * sx * sy;
	}

	// 3オクターブ合成（シェーダの DissolveNoise と同一）
	static float DissolveNoiseCPU(float ux, float uy) {
		return NoiseValue(ux * 5.f, uy * 5.f) * 0.50f
			+ NoiseValue(ux * 10.f, uy * 10.f) * 0.30f
			+ NoiseValue(ux * 20.f, uy * 20.f) * 0.20f;
	}
} // namespace

// 中心(x=0.5)が低値、左右端が高値のノイズテクスチャを生成
// → dissolveThreshold を 0→1 と上げると中心から端へ向かって消えていく
uint32_t TextureManager::CreateDissolveNoiseTexture() {
	const std::string key = "__DISSOLVE_NOISE__";
	if (textureData_.contains(key)) {
		return textureData_[key].srvIndex;
	}

	constexpr uint32_t W = 256;
	constexpr uint32_t H = 256;

	DirectX::TexMetadata metadata{};
	metadata.width = W;
	metadata.height = H;
	metadata.arraySize = 1;
	metadata.mipLevels = 1;
	metadata.format = DXGI_FORMAT_R8_UNORM;
	metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

	DirectX::ScratchImage image{};
	HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8_UNORM, W, H, 1, 1);
	assert(SUCCEEDED(hr));

	const DirectX::Image* img = image.GetImage(0, 0, 0);
	uint8_t* pixels = img->pixels;
	size_t rowPitch = img->rowPitch;

	for (uint32_t y = 0; y < H; ++y) {
		for (uint32_t x = 0; x < W; ++x) {
			float uvx = static_cast<float>(x) / static_cast<float>(W - 1);
			float uvy = static_cast<float>(y) / static_cast<float>(H - 1);
			float base = std::abs(uvx - 0.5f) * 2.0f; // 0 at center, 1 at edges
			float noise = DissolveNoiseCPU(uvx, uvy);
			float value = std::clamp(base * 0.60f + noise * 0.40f, 0.0f, 1.0f);
			pixels[y * rowPitch + x] = static_cast<uint8_t>(value * 255.0f);
		}
	}

	TextureData texData{};
	texData.srvIndex = srvManager_->Allocate();
	texData.metadata = metadata;
	texData.resource = dxManager_->GetResourceFactory()->CreateTexture2D(metadata);

	dxManager_->UploadTextureData(texData.resource.Get(), image);

	texData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(texData.srvIndex);
	texData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(texData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	dxManager_->GetDevice()->CreateShaderResourceView(texData.resource.Get(), &srvDesc, texData.srvHandleCPU);

	textureData_[key] = texData;
	dissolveNoiseIndex_ = texData.srvIndex;

	return texData.srvIndex;
}

// 方向性のない全面ランダムなノイズテクスチャを生成
// → dissolveThreshold を上げると、モデル全体がランダムにちぎれるように消えていく
uint32_t TextureManager::CreateRandomDissolveNoiseTexture() {
	const std::string key = "__DISSOLVE_NOISE_RANDOM__";
	if (textureData_.contains(key)) {
		return textureData_[key].srvIndex;
	}

	constexpr uint32_t W = 256;
	constexpr uint32_t H = 256;

	DirectX::TexMetadata metadata{};
	metadata.width = W;
	metadata.height = H;
	metadata.arraySize = 1;
	metadata.mipLevels = 1;
	metadata.format = DXGI_FORMAT_R8_UNORM;
	metadata.dimension = DirectX::TEX_DIMENSION_TEXTURE2D;

	DirectX::ScratchImage image{};
	HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8_UNORM, W, H, 1, 1);
	assert(SUCCEEDED(hr));

	const DirectX::Image* img = image.GetImage(0, 0, 0);
	uint8_t* pixels = img->pixels;
	size_t rowPitch = img->rowPitch;

	for (uint32_t y = 0; y < H; ++y) {
		for (uint32_t x = 0; x < W; ++x) {
			float uvx = static_cast<float>(x) / static_cast<float>(W - 1);
			float uvy = static_cast<float>(y) / static_cast<float>(H - 1);
			// 高周波の3オクターブ合成（グラデーション無しの純ノイズ）
			float noise = NoiseValue(uvx * 8.0f, uvy * 8.0f) * 0.50f
				+ NoiseValue(uvx * 16.0f, uvy * 16.0f) * 0.30f
				+ NoiseValue(uvx * 32.0f, uvy * 32.0f) * 0.20f;
			// コントラストを広げて 0〜1 の閾値スイープ全体で満遍なく消えるようにする
			float value = std::clamp((noise - 0.2f) / 0.6f, 0.0f, 1.0f);
			pixels[y * rowPitch + x] = static_cast<uint8_t>(value * 255.0f);
		}
	}

	TextureData texData{};
	texData.srvIndex = srvManager_->Allocate();
	texData.metadata = metadata;
	texData.resource = dxManager_->GetResourceFactory()->CreateTexture2D(metadata);

	dxManager_->UploadTextureData(texData.resource.Get(), image);

	texData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(texData.srvIndex);
	texData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(texData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	dxManager_->GetDevice()->CreateShaderResourceView(texData.resource.Get(), &srvDesc, texData.srvHandleCPU);

	textureData_[key] = texData;
	randomDissolveNoiseIndex_ = texData.srvIndex;

	return texData.srvIndex;
}

void TextureManager::LoadTextureFromMemory(const std::string& fileName, const uint8_t* pixels, uint32_t width, uint32_t height) {
	const std::string filePath = "Resource/Images/" + fileName;
	if (textureData_.contains(filePath)) return;

	ASSERT_MSG(srvManager_->CanAllocate(),
		"[TextureManager] SRV スロットが満杯です。テクスチャをこれ以上登録できません。");

	DirectX::ScratchImage image{};
	HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
	ASSERT_MSG(SUCCEEDED(hr),
		"[TextureManager] ScratchImage の初期化に失敗しました (LoadTextureFromMemory)。");

	const DirectX::Image* img = image.GetImage(0, 0, 0);
	for (uint32_t y = 0; y < height; ++y) {
		std::memcpy(img->pixels + y * img->rowPitch, pixels + y * width * 4, width * 4);
	}

	TextureData texData{};
	texData.srvIndex = srvManager_->Allocate();
	texData.metadata = image.GetMetadata();
	texData.resource = dxManager_->GetResourceFactory()->CreateTexture2D(image.GetMetadata());
	dxManager_->UploadTextureData(texData.resource.Get(), image);

	texData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(texData.srvIndex);
	texData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(texData.srvIndex);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	dxManager_->GetDevice()->CreateShaderResourceView(texData.resource.Get(), &srvDesc, texData.srvHandleCPU);

	textureData_[filePath] = std::move(texData);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetDissolveNoiseSrvHandleGPU() {
	assert(textureData_.contains("__DISSOLVE_NOISE__"));
	return textureData_["__DISSOLVE_NOISE__"].srvHandleGPU;
}
