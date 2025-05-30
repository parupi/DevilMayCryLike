#include "SkinnedModel.h"
#include "Model/Animation/Skeleton.h"
#include "Model/Animation/SkinCluster.h"
#include "Model/Animation/Animation.h"
#include <TextureManager.h>
#include "ModelManager.h"
#include <Object3d.h>
#include <Renderer/ModelRenderer.h>

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
	animator_ = std::make_unique<Animation>();
	animator_->Initialize(this, fileName);

	// メッシュとマテリアルの作成
	for (auto& skinnedMeshData : modelData_.meshes) {
		auto mesh = std::make_unique<Mesh>();
		mesh->Initialize(GetDxManager(), GetSrvManager(), skinnedMeshData.meshData);
		meshes_.emplace_back(std::move(mesh));
	}

	for (auto& materialData : modelData_.materials) {
		auto material = std::make_unique<Material>();
		material->Initialize(GetDxManager(), GetSrvManager(), materialData);
		materials_.emplace_back(std::move(material));
	}

	// スキンクラスタ作成
	skinCluster_ = std::make_unique<SkinCluster>();
	const auto& skinClusterData = modelData_.skinClusterData;
	const auto& meshData = modelData_.meshes.at(0); // 複数メッシュがある場合は要拡張

	skinCluster_->Initialize(skeleton_->GetSkeletonData(), meshData, skinClusterData, GetDxManager(), GetSrvManager());

}

void SkinnedModel::Update()
{
	animationTime += 1.0f / 60.0f;
	animationTime = std::fmod(animationTime, animator_->GetAnimation().duration); // 最後までいったらリピート再生

	skeleton_->ApplyAnimation(animator_->GetAnimation(), animationTime);
	skeleton_->Update();
	skinCluster_->UpdateSkinCluster(skeleton_->GetSkeletonData());
}

void SkinnedModel::Draw()
{
	modelLoader_->GetDxManager()->GetCommandList()->SetGraphicsRootDescriptorTable(7, skinCluster_->GetSkinCluster().paletteSrvHandle.second);

	const auto& view = skinCluster_->GetSkinCluster().influenceBufferView;
	modelLoader_->GetDxManager()->GetCommandList()->IASetVertexBuffers(1, 1, &view);

	for (const auto& mesh : meshes_) {
		// このメッシュに対応するマテリアルを設定
		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->Draw();

		// メッシュを描画
		mesh->Draw();
	}
}

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
						//object->SetModel(modelNames[selectedIndex]);
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
