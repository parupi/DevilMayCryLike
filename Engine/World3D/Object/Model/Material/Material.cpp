#include "Material.h"
#include "Graphics/Resource/TextureManager.h"

Material::Material() {

}

Material::~Material() {
}

void Material::Initialize(DirectXManager* directXManager, SrvManager* srvManager, MaterialData materialData) {
	directXManager_ = directXManager;
	srvManager_ = srvManager;
	materialData_ = materialData;

	CreateMaterialResource();
	CreateGBufferMaterialResource();
}

void Material::Update(const Vector3& objectScale) {
	// UV行列の材料はほとんど変化しない。毎フレーム3回の行列合成をやり直さないよう、
	// 入力が前回と同じならそのまま返す（マテリアル数はオブジェクト数より多いので効く）
	UVSource source{};
	source.uv = uvData_;
	source.objectScale = objectScale;
	source.enableTextureDensity = enableTextureDensity_ ? 1u : 0u;
	if (uvSourceValid_ && std::memcmp(&source, &cachedUVSource_, sizeof(UVSource)) == 0) {
		return;
	}
	cachedUVSource_ = source;
	uvSourceValid_ = true;

	Vector2 finalUVScale = uvData_.scale;

	// TextureDensity維持
	if (enableTextureDensity_) {
		finalUVScale.x = objectScale.x + objectScale.y + objectScale.z;
		finalUVScale.y = objectScale.x + objectScale.y + objectScale.z;
	}
	// uvTransformに値を適用
	uvTransform_.translate = {uvData_.position.x, uvData_.position.y, 0.0f};
	uvTransform_.rotate = {0.0f, 0.0f, uvData_.rotation};
	uvTransform_.scale = {finalUVScale.x, finalUVScale.y, 1.0f};
	// Transform情報を作る
	Matrix4x4 uvTransformMatrix = MakeIdentity4x4();
	uvTransformMatrix *= MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	// マテリアル構造体に反映
	materialForGPU_->uvTransform = uvTransformMatrix;
	gBufferMaterialParam_->uvTransform = uvTransformMatrix;
}

void Material::Bind(UINT RootParameterIndex) {
	D3D12_GPU_VIRTUAL_ADDRESS addr = directXManager_->GetResourceManager()->GetGPUVirtualAddress(materialHandle_);
	// マテリアルCBufferの場所を指定
	directXManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, addr);

	srvManager_->SetGraphicsRootDescriptorTable(RootParameterIndex, materialData_.textureIndex);
}

void Material::BindForGBuffer() {
	D3D12_GPU_VIRTUAL_ADDRESS addr = directXManager_->GetResourceManager()->GetGPUVirtualAddress(materialGBufferHandle_);
	directXManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, addr);
	srvManager_->SetGraphicsRootDescriptorTable(2, materialData_.textureIndex);
	// モデルのディゾルブは方向性のないランダムノイズを使う（中心→端のノイズはスプライト用）
	srvManager_->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance().GetRandomDissolveNoiseSrvIndex());
}

#ifdef _DEBUG
void Material::DebugGui(uint32_t index) {

	std::string label = "Material" + std::to_string(index);
	if (ImGui::TreeNode(label.c_str())) {
		if (ImGui::Button("ResetUVRotate")) {
			uvData_.rotation = 0.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("ResetUVScale")) {
			uvData_.scale = {1.0f, 1.0f};
		}
		ImGui::SameLine();
		if (ImGui::Button("ResetuvPosition")) {
			uvData_.position = {0.0f, 0.0f};
		}

		ImGui::DragFloat2("uvPosition", &uvData_.position.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvData_.scale.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvData_.rotation);
		// GBuffer(遅延描画)側にも同じ色を流さないと見た目が変わらないので両方に反映する
		if (ImGui::ColorEdit4("color", &materialForGPU_->color.x)) {
			gBufferMaterialParam_->materialColor = materialForGPU_->color;
		}
		ImGui::SliderFloat("environmentIntensity", &materialForGPU_->environmentIntensity, 0.0f, 1.0f);
		ImGui::Separator();
		ImGui::Text("--- Dissolve ---");
		ImGui::SliderFloat("dissolveThreshold", &gBufferMaterialParam_->dissolveThreshold, -1.0f, 1.0f);
		ImGui::SliderFloat("dissolveEdgeWidth", &gBufferMaterialParam_->dissolveEdgeWidth, 0.0f, 0.3f);
		ImGui::ColorEdit3("dissolveEdgeColor", &gBufferMaterialParam_->dissolveEdgeColor.x);
		ImGui::SliderFloat("dissolveEdgeIntensity", &gBufferMaterialParam_->dissolveEdgeColor.w, 0.0f, 20.0f);
		ImGui::TreePop();
	}
}
#endif // _DEBUG


void Material::CreateMaterialResource() {
	auto* resourceManager = directXManager_->GetResourceManager();
	// マテリアル用のリソースを作る。今回はFcolor1つ分のサイズを用意する
	materialHandle_ = resourceManager->CreateUploadBuffer(sizeof(MaterialForGPU), L"Material");
	void* ptr = resourceManager->Map(materialHandle_);
	assert(ptr);
	materialForGPU_ = reinterpret_cast<MaterialForGPU*>(ptr);
	// mtl等から読んだ基本色（テクスチャ無しならKd、テクスチャ付きなら白）
	materialForGPU_->color = materialData_.baseColor;
	materialForGPU_->enableLighting = true;
	materialForGPU_->uvTransform = MakeIdentity4x4();
	materialForGPU_->shininess = 50.0f;
	materialForGPU_->environmentIntensity = 0.01f;
}

void Material::CreateGBufferMaterialResource() {
	auto* resourceManager = directXManager_->GetResourceManager();

	materialGBufferHandle_ = resourceManager->CreateUploadBuffer(sizeof(GBufferMaterialParam), L"MaterialGBuffer");
	void* ptr = resourceManager->Map(materialGBufferHandle_);
	assert(ptr);
	gBufferMaterialParam_ = reinterpret_cast<GBufferMaterialParam*>(ptr);
	gBufferMaterialParam_->metal = 0.0f;
	gBufferMaterialParam_->roughness = 1.0f;
	gBufferMaterialParam_->dissolveThreshold = -1.0f;
	gBufferMaterialParam_->dissolveEdgeWidth = 0.05f;
	gBufferMaterialParam_->dissolveEdgeColor = { 1.0f, 0.3f, 0.0f, 8.0f };
	gBufferMaterialParam_->materialColor = materialData_.baseColor;
}
