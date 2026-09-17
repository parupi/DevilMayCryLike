#include "PrimitiveRenderer.h"
#include "PrimitiveFactory.h"
#include "RendererManager.h"
#include <Graphics/Rendering/Sky/SkySystem.h>
#include "Graphics/Resource/TextureManager.h"

#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI
PrimitiveRenderer::PrimitiveRenderer(const std::string& renderName, PrimitiveType type, std::string textureName) {
	name_ = renderName;
	localTransform_ = std::make_unique<WorldTransform>();
	localTransform_->Initialize();

	// Primitiveモデルを生成
	model_ = PrimitiveFactory::Create(type, textureName); // Plane, Ring, Cylinder を返す
}

PrimitiveRenderer::~PrimitiveRenderer() {
	model_.reset();
	localTransform_.reset();
}

void PrimitiveRenderer::Update(WorldTransform* parentTransform) {
	BaseCamera* camera = CameraManager::GetInstance().GetActiveCamera();
	if (!camera) return;

	model_->Update();

	if (localTransform_->GetParent() == nullptr) {
		localTransform_->SetParent(parentTransform);
	}
	localTransform_->TransferMatrix(CameraManager::GetInstance().GetCurrentCamera());
}

void PrimitiveRenderer::Draw() {
	localTransform_->BindToShader(RendererManager::GetInstance().GetDxManager()->GetCommandList(), 4);
	// 環境マップバインド
	int envMapIndex = SkySystem::GetInstance().GetEnvironmentMapIndex();

	if (envMapIndex < 0) {
		TextureManager::GetInstance().LoadTexture("skybox_cube.dds");
		envMapIndex = TextureManager::GetInstance().GetTextureIndexByFilePath("skybox_cube.dds");
	}
	RendererManager::GetInstance().GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);

	model_->Draw();
}

void PrimitiveRenderer::DrawGBuffer() {
	auto* cmd = RendererManager::GetInstance().GetDxManager()->GetCommandList();
	localTransform_->BindToShader(cmd, 1);

	// レンダラー単位のDissolve上書き+エミッシブティント（b2ルート定数）。無効時も毎ドロー設定して前の値が残らないようにする
	const float dissolveConstants[12] = {
		dissolveThreshold_, dissolveEdgeWidth_, 0.0f, 0.0f,
		dissolveEdgeColor_.x, dissolveEdgeColor_.y, dissolveEdgeColor_.z, dissolveEdgeColor_.w,
		emissiveTint_.x, emissiveTint_.y, emissiveTint_.z, emissiveTint_.w,
	};
	cmd->SetGraphicsRoot32BitConstants(4, 12, dissolveConstants, 0);

	model_->DrawGBuffer(); // Model側へ委譲
}

#ifdef _DEBUG
void PrimitiveRenderer::DebugGui(size_t index) {
	std::string label = "TransformRender" + std::to_string(index);
	if (ImGui::TreeNode(label.c_str())) {
		localTransform_->DebugGui();
		model_->DebugGuiPrimitive();
		ImGui::TreePop();
	}
}
#endif