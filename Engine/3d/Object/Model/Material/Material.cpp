#include "Material.h"
#include <base/TextureManager.h>

Material::Material()
{

}

Material::~Material()
{
	//if (materialGBufferHandle_ != kInvalidBufferHandle) {
	//	resourceManager_->ReleaseBuffer(materialGBufferHandle_);
	//	materialGBufferHandle_ = kInvalidBufferHandle;
	//}
	//if (materialBufferHandle_ != kInvalidBufferHandle) {
	//	resourceManager_->ReleaseBuffer(materialBufferHandle_);
	//	materialBufferHandle_ = kInvalidBufferHandle;
	//}
}

void Material::Initialize(DirectXManager* directXManager, SrvManager* srvManager, MaterialData materialData)
{
	directXManager_ = directXManager;
	srvManager_ = srvManager;
	materialData_ = materialData;

	CreateMaterialResource();
	CreateGBufferMaterialResource();
}

void Material::Update()
{
	// uvTransformに値を適用
	uvTransform_.translate = { uvData_.position.x, uvData_.position.y, 0.0f };
	uvTransform_.rotate = { 0.0f, 0.0f, uvData_.rotation };
	uvTransform_.scale = { uvData_.size.x, uvData_.size.y, 1.0f };
	// Transform情報を作る
	Matrix4x4 uvTransformMatrix = MakeIdentity4x4();
	uvTransformMatrix *= MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	materialForGPU_->uvTransform = uvTransformMatrix;
}

void Material::Bind()
{
	D3D12_GPU_VIRTUAL_ADDRESS addr = directXManager_->GetResourceManager()->GetGPUVirtualAddress(materialHandle_);
	// マテリアルCBufferの場所を指定
	directXManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, addr);

	srvManager_->SetGraphicsRootDescriptorTable(2, materialData_.textureIndex);
}

void Material::BindForGBuffer()
{
	D3D12_GPU_VIRTUAL_ADDRESS addr = directXManager_->GetResourceManager()->GetGPUVirtualAddress(materialGBufferHandle_);
	directXManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, addr);
	srvManager_->SetGraphicsRootDescriptorTable(2, materialData_.textureIndex);
}

#ifdef _DEBUG
void Material::DebugGui(uint32_t index)
{
	std::string label = "Material" + std::to_string(index);
	if (ImGui::TreeNode(label.c_str())) {
		if (ImGui::Button("ResetUVRotate")) {
			uvData_.rotation = 0.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("ResetUVScale")) {
			uvData_.size = {1.0f, 1.0f};
		}
		ImGui::SameLine();
		if (ImGui::Button("ResetuvPosition")) {
			uvData_.position = {0.0f, 0.0f};
		}

		ImGui::DragFloat2("uvPosition", &uvData_.position.x, 0.01f, -10.0f, 10.0f);
		ImGui::DragFloat2("UVScale", &uvData_.size.x, 0.01f, -10.0f, 10.0f);
		ImGui::SliderAngle("UVRotate", &uvData_.rotation);
		ImGui::ColorEdit4("color", &materialForGPU_->color.x);
		ImGui::SliderFloat("environmentIntensity", &materialForGPU_->environmentIntensity, 0.0f, 1.0f);
		ImGui::TreePop();
	}
}
#endif // _DEBUG


void Material::CreateMaterialResource()
{
	auto* resourceManager = directXManager_->GetResourceManager();
	// マテリアル用のリソースを作る。今回はFcolor1つ分のサイズを用意する
	materialHandle_ = resourceManager->CreateUploadBuffer(sizeof(MaterialForGPU), L"Material");
	void* ptr = resourceManager->Map(materialHandle_);
	assert(ptr);
	materialForGPU_ = reinterpret_cast<MaterialForGPU*>(ptr);
	materialForGPU_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialForGPU_->enableLighting = true;
	materialForGPU_->uvTransform = MakeIdentity4x4();
	materialForGPU_->shininess = 50.0f;
	materialForGPU_->environmentIntensity = 0.01f; 

	// MaterialData から反映
	//materialForGPU_->color.x = materialData_.Kd.r;                 // 拡散反射色を使用
	//materialForGPU_->color.y = materialData_.Kd.g;                 // 拡散反射色を使用
	//materialForGPU_->color.z = materialData_.Kd.r;                 // 拡散反射色を使用
	//materialForGPU_->enableLighting = true;
	//materialForGPU_->uvTransform = MakeIdentity4x4();
	//materialForGPU_->shininess = materialData_.Ns;             // 鏡面反射強度を反映
}

void Material::CreateGBufferMaterialResource()
{
	auto* resourceManager = directXManager_->GetResourceManager();

	materialGBufferHandle_ = resourceManager->CreateUploadBuffer(sizeof(GBufferMaterialParam), L"MaterialGBuffer");
	void* ptr = resourceManager->Map(materialGBufferHandle_);
	assert(ptr);
	gBufferMaterialParam_ = reinterpret_cast<GBufferMaterialParam*>(ptr);
	gBufferMaterialParam_->metal = 0.0f;
	gBufferMaterialParam_->roughness = 1.0f;
}
