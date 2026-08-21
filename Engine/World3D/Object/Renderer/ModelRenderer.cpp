#include "ModelRenderer.h"
#include "RendererManager.h"
#include <World3D/Object/Model/ModelManager.h>
#include <World3D/Object/Model/SkinnedModel.h>
#include <World3D/Object/Object3dManager.h>
#include <Graphics/Rendering/Sky/SkySystem.h>
#include "World3D/Object/Model/ModelStructs.h"
#include "Math/MathUtils.h"
#include <Utility/TimeManager.h>
#include <algorithm>

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
	// Object3d と同じく、名前でのmap検索を避けて流し込み済みのカメラを使う
	camera_ = Object3dManager::GetInstance().GetDefaultCamera();
	if (!camera_) {
		camera_ = CameraManager::GetInstance().GetActiveCamera();
	}

	if (localTransform_->GetParent() == nullptr) {
		localTransform_->SetParent(parentTransform);
	}

	localTransform_->TransferMatrix(camera_);

	// アニメーションはゲーム時間で進める。
	// ヒットストップ中は GetGameDelta() が 0 になるのでポーズもそこで止まる
	if (skinnedInstance_) {
		skinnedInstance_->Update(TimeManager::GetGameDelta());
	}

	model_->Update(localTransform_->GetWorldScale());
}

void ModelRenderer::Draw() {
	localTransform_->BindToShader(RendererManager::GetInstance().GetDxManager()->GetCommandList(), 4);
	// 環境マップバインド
	int envMapIndex = SkySystem::GetInstance().GetEnvironmentMapIndex();

	if (envMapIndex >= 0) {
		RendererManager::GetInstance().GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	}
	else {
		TextureManager::GetInstance().LoadTexture("skybox_cube.dds");
		envMapIndex = TextureManager::GetInstance().GetTextureIndexByFilePath("skybox_cube.dds");
		RendererManager::GetInstance().GetSrvManager()->SetGraphicsRootDescriptorTable(6, envMapIndex);
	}

	if (skinnedInstance_) {
		static_cast<SkinnedModel*>(model_)->DrawWith(skinnedInstance_.get());
	} else {
		model_->Draw();
	}
}

void ModelRenderer::DrawGBuffer() {
	auto* cmd = RendererManager::GetInstance().GetDxManager()->GetCommandList();
	localTransform_->BindToShader(cmd, 1);

	// レンダラー単位のDissolve上書き+エミッシブティント（b2ルート定数）。無効時も毎ドロー設定して前の値が残らないようにする
	const float dissolveConstants[12] = {
		dissolveThreshold_, dissolveEdgeWidth_, 0.0f, 0.0f,
		dissolveEdgeColor_.x, dissolveEdgeColor_.y, dissolveEdgeColor_.z, dissolveEdgeColor_.w,
		emissiveTint_.x, emissiveTint_.y, emissiveTint_.z, emissiveTint_.w,
	};
	cmd->SetGraphicsRoot32BitConstants(4, 12, dissolveConstants, 0);

	if (skinnedInstance_) {
		static_cast<SkinnedModel*>(model_)->DrawGBufferWith(skinnedInstance_.get());
	} else {
		model_->DrawGBuffer(); // Model側へ委譲
	}
}

void ModelRenderer::DrawShadow() {
	auto* commandList = RendererManager::GetInstance().GetDxManager()->GetCommandList();

	localTransform_->BindToShader(commandList, 0);

	if (skinnedInstance_) {
		static_cast<SkinnedModel*>(model_)->DrawShadowWith(skinnedInstance_.get());
	} else {
		model_->DrawShadow();
	}
}

bool ModelRenderer::GetShadowBoundingSphere(Vector3& outCenter, float& outRadius) const {
	// スキンモデルはポーズで形が変わるので、バインドポーズのAABBで弾くと消えてしまう
	if (skinnedInstance_ || !model_ || !localTransform_) return false;

	Vector3 localMin{};
	Vector3 localMax{};
	if (!model_->GetLocalBounds(localMin, localMax)) return false;

	const Matrix4x4& world = localTransform_->GetMatWorld();
	outCenter = Transform((localMin + localMax) * 0.5f, world);

	// 行ベクトル規約なので、上3行がスケール込みの基底。一番伸びている軸で半径を膨らませる
	float maxAxisScale = 0.0f;
	for (int row = 0; row < 3; ++row) {
		const Vector3 axis{ world.m[row][0], world.m[row][1], world.m[row][2] };
		maxAxisScale = (std::max)(maxAxisScale, Length(axis));
	}
	outRadius = Length((localMax - localMin) * 0.5f) * maxAxisScale;
	return true;
}

void ModelRenderer::SetModel(const std::string& filePath) {
	// モデルを検索してセットする
	model_ = ModelManager::GetInstance().FindModel(filePath);

	// スキンモデルなら、このレンダラー専用のポーズと出力頂点バッファを作る。
	// モデルを差し替えたら前のインスタンスは捨てる（ディスクリプタもデストラクタで返る）
	skinnedInstance_.reset();
	if (auto* skinned = dynamic_cast<SkinnedModel*>(model_)) {
		skinnedInstance_ = skinned->CreateInstance();
	}
}

AnimationPlayer* ModelRenderer::GetAnimationPlayer() const {
	return skinnedInstance_ ? skinnedInstance_->GetPlayer() : nullptr;
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


