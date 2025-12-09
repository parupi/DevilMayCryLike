#include "ModelRenderer.h"
#include "RendererManager.h"
#include <3d/Object/Model/ModelManager.h>
#include <3d/SkySystem/SkySystem.h>
#include "3d/Object/Model/ModelStructs.h"

ModelRenderer::ModelRenderer(const std::string& renderName, const std::string& filePath)
{
	localTransform_ = std::make_unique<WorldTransform>();
	localTransform_->Initialize();
	SetModel(filePath);
	name_ = renderName;
}

void ModelRenderer::Update(WorldTransform* parentTransform)
{
	camera_ = CameraManager::GetInstance()->GetActiveCamera();
	model_->Update();

	if (localTransform_->GetParent() == nullptr) {
		localTransform_->SetParent(parentTransform);
	}
	
	localTransform_->TransferMatrix();
	

	//Matrix4x4 worldViewProjectionMatrix;
	//if (camera_) {
	//	const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
	//	worldViewProjectionMatrix = localTransform_->GetMatWorld() * viewProjectionMatrix;
	//} else {
	//	worldViewProjectionMatrix = localTransform_->GetMatWorld();
	//}

	//localTransform_->SetMapWVP(worldViewProjectionMatrix);
	//localTransform_->SetMapWorld(localTransform_->GetMatWorld());
}

void ModelRenderer::Draw()
{
	//RendererManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, localTransform_->GetConstBuffer()->GetGPUVirtualAddress());
	localTransform_->BindToShader(RendererManager::GetInstance()->GetDxManager()->GetCommandList(), 4);
	// 環境マップバインド
	int envMapIndex = SkySystem::GetInstance()->GetEnvironmentMapIndex();

	if (envMapIndex >= 0) {
		RendererManager::GetInstance()->GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	} else {
		TextureManager::GetInstance()->LoadTexture("skybox_cube.dds");
		envMapIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath("skybox_cube.dds");
		RendererManager::GetInstance()->GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	}

	model_->Draw();
}

void ModelRenderer::DrawGBuffer()
{
	// (transform / material bind)
	//RendererManager::GetInstance()->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, localTransform_->GetConstBuffer()->GetGPUVirtualAddress());
	localTransform_->BindToShader(RendererManager::GetInstance()->GetDxManager()->GetCommandList(), 1);
	model_->DrawGBuffer(); // Model側へ委譲
}

void ModelRenderer::SetModel(const std::string& filePath)
{
	// モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}

#ifdef _DEBUG
void ModelRenderer::DebugGui(size_t index)
{
	std::string label = "TransformRender" + std::to_string(index);
	if (ImGui::TreeNode(label.c_str())) {
		localTransform_->DebugGui();
		model_->DebugGui(this);
		ImGui::TreePop();
	}
}
#endif // _DEBUG


