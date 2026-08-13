#include "SkinnedModel.h"
#include "World3D/Object/Model/Animation/SkinnedInstance.h"
#include "World3D/Object/Model/Animation/SkinningResource.h"
#include "Graphics/Resource/TextureManager.h"
#include "World3D/Object/Model/ModelManager.h"
#include <World3D/Object/Object3d.h>
#include <World3D/Object/Renderer/ModelRenderer.h>
#include <DirectXTex/d3dx12.h>
#include <World3D/Light/LightManager.h>
#include "World3D/Object/Object3dManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"

void SkinnedModel::Initialize(ModelLoader* modelLoader, const std::string& fileName)
{
	// モデルローダーの保持
	modelLoader_ = modelLoader;
	modelName_ = fileName;

	// モデルとアニメーションクリップを1回のgltf読み込みでまとめて取る
	modelData_ = modelLoader_->LoadSkinnedModel(fileName, &clipSet_);
	// イベント定義（任意の .anim.json）はクリップを読んだ後に足す
	clipSet_.LoadEvents(fileName);

	// バインドポーズのスケルトン作成
	bindSkeleton_.BuildFromNode(modelData_.rootNode);
	bindSkeleton_.Update();

	// メッシュとスキニング入力リソースの作成
	for (size_t i = 0; i < modelData_.meshes.size(); ++i) {
		const auto& skinnedMeshData = modelData_.meshes[i];

		auto mesh = std::make_unique<Mesh>();
		mesh->Initialize(GetDxManager(), GetSrvManager(), skinnedMeshData);
		mesh->CreateSkinningResource(bindSkeleton_.GetSkeletonData(), skinnedMeshData, skinnedMeshData.skinClusterData);

		meshes_.emplace_back(std::move(mesh));
	}

	BuildInverseBindPoseMatrices();

	for (auto& materialData : modelData_.materials) {
		auto material = std::make_unique<Material>();
		material->Initialize(GetDxManager(), GetSrvManager(), materialData);
		materials_.emplace_back(std::move(material));
	}
}

void SkinnedModel::BuildInverseBindPoseMatrices()
{
	const SkeletonData& skeletonData = bindSkeleton_.GetSkeletonData();
	inverseBindPoseMatrices_.assign(skeletonData.joints.size(), MakeIdentity4x4());

	for (const auto& meshData : modelData_.meshes) {
		for (const auto& [jointName, weightData] : meshData.skinClusterData) {
			auto found = skeletonData.jointMap.find(jointName);
			if (found == skeletonData.jointMap.end()) continue;
			// 同じジョイントを複数メッシュが参照していても値は同じなので上書きで問題ない
			inverseBindPoseMatrices_[found->second] = weightData.inverseBindPoseMatrix;
		}
	}
}

std::unique_ptr<SkinnedInstance> SkinnedModel::CreateInstance()
{
	auto instance = std::make_unique<SkinnedInstance>();
	instance->Initialize(this);
	return instance;
}

void SkinnedModel::Update(const Vector3& objectScale)
{
	// マテリアルはアセット側なので共有。ポーズは SkinnedInstance::Update が進める
	for (size_t i = 0; i < materials_.size(); i++) {
		materials_[i]->Update(objectScale);
	}
}

void SkinnedModel::DrawWith(SkinnedInstance* instance)
{
	auto* cmd = modelLoader_->GetDxManager()->GetCommandList();

	for (size_t i = 0; i < meshes_.size(); ++i) {
		auto& mesh = meshes_[i];

		CameraManager::GetInstance().BindCameraToShader();
		LightManager::GetInstance().BindLightsToShader();

		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->Bind(5);

		mesh->Bind(&instance->GetOutputVBV(i));

		cmd->DrawIndexedInstanced(UINT(mesh->GetMeshData().indices.size()), 1, 0, 0, 0);
	}
}

void SkinnedModel::DrawGBufferWith(SkinnedInstance* instance)
{
	// CSスキニング後の出力頂点バッファは通常のVertexDataレイアウトなので、
	// GBufferのシェーダーは静的モデルとまったく同じものが使える
	auto* cmd = modelLoader_->GetDxManager()->GetCommandList();

	for (size_t i = 0; i < meshes_.size(); ++i) {
		auto& mesh = meshes_[i];

		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->BindForGBuffer();

		mesh->Bind(&instance->GetOutputVBV(i));

		cmd->DrawIndexedInstanced(UINT(mesh->GetMeshData().indices.size()), 1, 0, 0, 0);
	}
}

void SkinnedModel::DrawShadowWith(SkinnedInstance* instance)
{
	auto* cmd = modelLoader_->GetDxManager()->GetCommandList();

	for (size_t i = 0; i < meshes_.size(); ++i) {
		auto& mesh = meshes_[i];
		mesh->Bind(&instance->GetOutputVBV(i));
		cmd->DrawIndexedInstanced(UINT(mesh->GetMeshData().indices.size()), 1, 0, 0, 0);
	}
}

std::vector<Material*> SkinnedModel::GetMaterials()
{
	std::vector<Material*> materials;

	for (auto& material : materials_) {
		materials.push_back(material.get());
	}

	return materials;
}

#ifdef _DEBUG
void SkinnedModel::DebugGui(ModelRenderer* render)
{
	if (ImGui::TreeNode("Models")) {
		auto& modelMap = ModelManager::GetInstance().skinnedModels;
		static std::vector<std::string> modelNames;
		static int selectedIndex = 0;

		// モデル一覧を初期化（必要なら一度だけでOK）
		if (modelNames.empty()) {
			for (const auto& pair : modelMap) {
				modelNames.push_back(pair.first);
			}
		}

		if (!modelNames.empty()) {
			const char* currentItem = modelNames[selectedIndex].c_str();

			if (ImGui::BeginCombo("Model List", currentItem)) {
				for (int i = 0; i < modelNames.size(); ++i) {
					bool isSelected = (selectedIndex == i);
					if (ImGui::Selectable(modelNames[i].c_str(), isSelected)) {
						selectedIndex = i;
						render->SetModel(modelNames[selectedIndex]);
					}
					if (isSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::TreePop();
	}
}
#endif
