#include "Material.h"
#include "base/DirectXManager.h"
#include "base/SrvManager.h"
#include <base/TextureManager.h>

Material::Material()
{

}

void Material::Initialize(DirectXManager* directXManager, SrvManager* srvManager, MaterialData materialData)
{
	directXManager_ = directXManager;
	srvManager_ = srvManager;
	materialData_ = materialData;

	CreateMaterialResource();
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
	// マテリアルCBufferの場所を指定
	directXManager_->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

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
		ImGui::TreePop();
	}
}
#endif // _DEBUG


void Material::CreateMaterialResource()
{
	// マテリアル用のリソースを作る。今回はFcolor1つ分のサイズを用意する
	materialResource_ = directXManager_->CreateBufferResource(sizeof(MaterialForGPU));
	// 書き込むためのアドレスを取得
	materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialForGPU_));
	// 白を入れる
	materialForGPU_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialForGPU_->enableLighting = true;
	materialForGPU_->uvTransform = MakeIdentity4x4();
	materialForGPU_->shininess = 20.0f;

	//// MaterialData から反映
	//materialForGPU_->color.x = materialData_.Kd.r;                 // 拡散反射色を使用
	//materialForGPU_->color.y = materialData_.Kd.g;                 // 拡散反射色を使用
	//materialForGPU_->color.z = materialData_.Kd.r;                 // 拡散反射色を使用
	//materialForGPU_->enableLighting = true;
	//materialForGPU_->uvTransform = MakeIdentity4x4();
	//materialForGPU_->shininess = materialData_.Ns;             // 鏡面反射強度を反映
}
