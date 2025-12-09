#include "SkinnedModel.h"
#include "3d/Object/Model/Animation/Skeleton.h"
#include "3d/Object/Model/Animation/SkinCluster.h"
#include "3d/Object/Model/Animation/Animation.h"
#include "Graphics/Resource/TextureManager.h"
#include "3d/Object/Model/ModelManager.h"
#include <3d/Object/Object3d.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <DirectXTex/d3dx12.h>
#include <3d/Light/LightManager.h>
#include "3d/Object/Object3dManager.h"
#include "Graphics/Rendering/PSO/PSOManager.h"

void SkinnedModel::Initialize(ModelLoader* modelLoader, const std::string& fileName)
{
	// モデルローダーの保持
	modelLoader_ = modelLoader;

	// モデルの読み込み
	modelData_ = modelLoader_->LoadSkinnedModel(fileName);

	// スケルトン作成
	skeleton_ = std::make_unique<Skeleton>();
	skeleton_->Initialize(this);

	// アニメーション作成
	animation_ = std::make_unique<Animation>();
	animation_->Initialize(this, fileName);

	// メッシュとマテリアルの作成
	for (size_t i = 0; i < modelData_.meshes.size(); ++i) {
		const auto& skinnedMeshData = modelData_.meshes[i];

		auto mesh = std::make_unique<Mesh>();
		mesh->Initialize(GetDxManager(), GetSrvManager(), skinnedMeshData);

		// 各メッシュにスキンクラスタを作成させる
		mesh->CreateSkinCluster(skeleton_->GetSkeletonData(), skinnedMeshData, skinnedMeshData.skinClusterData);

		meshes_.emplace_back(std::move(mesh));
	}

	for (auto& materialData : modelData_.materials) {
		auto material = std::make_unique<Material>();
		material->Initialize(GetDxManager(), GetSrvManager(), materialData);
		materials_.emplace_back(std::move(material));
	}
}

void SkinnedModel::Update()
{
	// アニメーションの更新を呼ぶ
	animation_->Update();

	// アニメーションの時間取得
	animationTime = animation_->GetAnimationTime();

	skeleton_->Update();


	for (const auto& mesh : meshes_) {
		auto* cluster = mesh->GetSkinCluster();
		cluster->UpdateInputVertex(mesh->GetSkinnedMeshData()); // メッシュ単位になったのでこれでOK
		cluster->UpdateSkinCluster(skeleton_->GetSkeletonData());
	}
}

void SkinnedModel::Draw()
{
	for (auto& mesh : meshes_) {
		CameraManager::GetInstance()->BindCameraToShader();
		LightManager::GetInstance()->BindLightsToShader();

		// マテリアル設定
		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->Bind();

		// 描画
		mesh->Bind();

		modelLoader_->GetDxManager()->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetMeshData().indices.size()), 1, 0, 0, 0);
	}
}

void SkinnedModel::UpdateSkinningWithCS()
{
	auto* commandList = modelLoader_->GetDxManager()->GetCommandList();

	// Compute用のPSOとRootSignature設定
	commandList->SetPipelineState(Object3dManager::GetInstance()->GetPsoManager()->GetSkinningPSO());
	commandList->SetComputeRootSignature(Object3dManager::GetInstance()->GetPsoManager()->GetSkinningSignature());

	// 各スキンクラスタのスキニング処理
	for (auto& mesh : meshes_) {
		mesh->Update();
	}
}


void SkinnedModel::DrawGBuffer()
{
}

#ifdef _DEBUG
void SkinnedModel::DebugGui(ModelRenderer* render)
{
	if (ImGui::TreeNode("Models")) {
		auto& modelMap = ModelManager::GetInstance()->skinnedModels;
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
