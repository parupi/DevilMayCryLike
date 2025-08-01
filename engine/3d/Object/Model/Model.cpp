#include "Model.h"
#include "math/function.h"
#include "base/TextureManager.h"
#include <math/Vector4.h>
#include <math/Vector2.h>
#include <3d/Object/Model/ModelManager.h>
#include <3d/WorldTransform.h>
#include <3d/Object/Model/Animation/Animation.h>
#include "Mesh/Mesh.h"
#include "3d/Object/Model/Material/Material.h"
#include <3d/Object/Object3d.h>
#include <3d/Object/Renderer/ModelRenderer.h>
#include <3d/Light/LightManager.h>

void Model::Initialize(ModelLoader* modelManager, const std::string& fileName)
{
	modelLoader_ = modelManager;

	// モデルの読み込み
	modelData_ = modelLoader_->LoadModelFile(fileName);

	// 各メッシュをMeshクラスへ変換
	for (const auto& meshData : modelData_.meshes) {
		auto mesh = std::make_unique<Mesh>();
		mesh->Initialize(modelLoader_->GetDxManager(), modelLoader_->GetSrvManager(), meshData);
		meshes_.push_back(std::move(mesh));
	}

	// 各メッシュをMeshクラスへ変換
	for (const auto& materialData : modelData_.materials) {
		auto material = std::make_unique<Material>();
		material->Initialize(modelLoader_->GetDxManager(), modelLoader_->GetSrvManager(), materialData);
		materials_.push_back(std::move(material));
	}
}

void Model::InitializeFromMesh(const MeshData& meshData, const MaterialData& materialData)
{
	modelLoader_ = ModelManager::GetInstance()->GetModelLoader();

	// Meshの生成と初期化
	auto mesh = std::make_unique<Mesh>();
	mesh->Initialize(modelLoader_->GetDxManager(), modelLoader_->GetSrvManager(), meshData);
	meshes_.push_back(std::move(mesh));

	// Materialの生成と初期化
	auto material = std::make_unique<Material>();
	material->Initialize(modelLoader_->GetDxManager(), modelLoader_->GetSrvManager(), materialData);
	materials_.push_back(std::move(material));
}

void Model::Update()
{
	for (size_t i = 0; i < materials_.size(); i++) {
		materials_[i]->Update();
	}

}

void Model::Draw()
{
	for (const auto& mesh : meshes_) {
		// このメッシュに対応するマテリアルを設定
		assert(mesh->GetMeshData().materialIndex < materials_.size());
		materials_[mesh->GetMeshData().materialIndex]->Bind();

		CameraManager::GetInstance()->BindCameraToShader();
		LightManager::GetInstance()->BindLightsToShader();

		// メッシュを描画
		mesh->Bind();
		modelLoader_->GetDxManager()->GetCommandList()->DrawIndexedInstanced(UINT(mesh->GetMeshData().indices.size()), 1, 0, 0, 0);
	}
}

void Model::Bind()
{
	    for (const auto& mesh : meshes_) {
        // メッシュに対応するマテリアルをバインド
        assert(mesh->GetMeshData().materialIndex < materials_.size());
        materials_[mesh->GetMeshData().materialIndex]->Bind();

        // メッシュをバインド（頂点バッファなど）
        mesh->Bind();
    }
}

#ifdef _DEBUG
void Model::DebugGui(ModelRenderer* render)
{
	if (ImGui::TreeNode("Models")) {
		auto& modelMap = ModelManager::GetInstance()->models;
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

	for (size_t i = 0; i < materials_.size(); i++) {
		if (i == 0) {
			continue;
		}
		materials_[i]->DebugGui(static_cast<uint32_t>(i));
	}
}

void Model::DebugGuiPrimitive()
{
	materials_[0]->DebugGui(static_cast<uint32_t>(1));
}

#endif // _DEBUG

