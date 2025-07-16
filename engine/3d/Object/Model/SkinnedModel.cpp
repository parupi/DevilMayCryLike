#include "SkinnedModel.h"
#include "3d/Object/Model/Animation/Skeleton.h"
#include "3d/Object/Model/Animation/SkinCluster.h"
#include "3d/Object/Model/Animation/Animation.h"
#include <base/TextureManager.h>
#include "3d/Object/Model/ModelManager.h"
#include <3d/Object/Object3d.h>
#include <3d/Object/Renderer/ModelRenderer.h>

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
	for (const auto& mesh : modelData_.meshes) {
		std::unique_ptr<SkinCluster> skinCluster = std::make_unique<SkinCluster>();
		skinCluster->Initialize(
			skeleton_->GetSkeletonData(),
			mesh, // 各メッシュごと
			mesh.skinClusterData,
			GetDxManager(),
			GetSrvManager()
		);
		skinClusters_.emplace_back(std::move(skinCluster));

	}
}

void SkinnedModel::Update()
{
	// アニメーションの更新を呼ぶ（ここが抜けていた）
	animation_->Update();

	// アニメーションの時間取得
	animationTime = animation_->GetAnimationTime();

	//skeleton_->ApplyAnimation(animation_->GetCurrentAnimation(), animationTime);
	skeleton_->Update();
	for (const auto& skinCluster : skinClusters_) {
		skinCluster->UpdateSkinCluster(skeleton_->GetSkeletonData());
	}
}

void SkinnedModel::Draw()
{
	for (size_t i = 0; i < meshes_.size(); ++i) {
		assert(i < skinClusters_.size());
		const auto& skinCluster = skinClusters_[i];

		// パレットSRVをバインド
		modelLoader_->GetDxManager()->GetCommandList()->SetGraphicsRootDescriptorTable(7, skinCluster->GetSkinCluster().paletteSrvHandle.second);

		// 頂点バッファ（インフルエンス）を設定
		const auto& view = skinCluster->GetSkinCluster().influenceBufferView;
		modelLoader_->GetDxManager()->GetCommandList()->IASetVertexBuffers(1, 1, &view);

		// マテリアル設定
		const auto& mesh = meshes_[i];
		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->Bind();

		// 描画
		mesh->Draw();
	}

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
