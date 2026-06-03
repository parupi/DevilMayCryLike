#include "ModelRenderer.h"
#include "RendererManager.h"
#include <World3D/Object/Model/ModelManager.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include "World3D/Object/Model/ModelStructs.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI
ModelRenderer::ModelRenderer(const std::string& renderName, const std::string& filePath) {
	localTransform_ = std::make_unique<WorldTransform>();
	localTransform_->Initialize();
	SetModel(filePath);
	name_ = renderName;
}

void ModelRenderer::Update(WorldTransform* parentTransform) {
	camera_ = CameraManager::GetInstance()->GetActiveCamera();

	if (localTransform_->GetParent() == nullptr) {
		localTransform_->SetParent(parentTransform);
	}

	localTransform_->TransferMatrix(camera_);

	model_->Update(localTransform_->GetWorldScale());
}

void ModelRenderer::Draw() {
	//RendererManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, localTransform_->GetConstBuffer()->GetGPUVirtualAddress());
	localTransform_->BindToShader(RendererManager::GetInstance()->GetDxManager()->GetCommandList(), 4);
	// 環境マップバインド
	int envMapIndex = SkySystem::GetInstance()->GetEnvironmentMapIndex();

	if (envMapIndex >= 0) {
		RendererManager::GetInstance()->GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	}
	else {
		TextureManager::GetInstance()->LoadTexture("skybox_cube.dds");
		envMapIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("skybox_cube.dds");
		RendererManager::GetInstance()->GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	}

	model_->Draw();
}

void ModelRenderer::DrawGBuffer() {
	localTransform_->BindToShader(RendererManager::GetInstance()->GetDxManager()->GetCommandList(), 1);
	model_->DrawGBuffer(); // Model側へ委譲
}

void ModelRenderer::DrawShadow() {
	auto* commandList = RendererManager::GetInstance()->GetDxManager()->GetCommandList();

	localTransform_->BindToShader(commandList, 0);

	model_->DrawShadow();
}

void ModelRenderer::SetModel(const std::string& filePath) {
	// モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

#ifdef _DEBUG
void ModelRenderer::DebugGui(size_t index) {
	std::string label = "TransformRender" + std::to_string(index);
	if (ImGui::TreeNode(label.c_str())) {
		localTransform_->DebugGui();
		model_->DebugGui(this);
		ImGui::TreePop();
	}
}
#endif // _DEBUG


