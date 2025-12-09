#include <3d/WorldTransform.h>
#include <math/function.h>
#include "Graphics/Device/DirectXManager.h"
#include <3d/Object/Object3dManager.h>
#include "Camera/CameraManager.h"

WorldTransform::~WorldTransform()
{
	//// GPUリソースの解放
	//if (constBuffer_) {
	//	constBuffer_->Unmap(0, nullptr);
	//	constBuffer_.Reset();
	//}

#ifdef _DEBUG
	Logger::Log("WorldTransform resources released.\n");
#endif
}

void WorldTransform::Initialize()
{
	// ワールド行列の初期化
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 定数バッファ生成
	CreateConstBuffer();

	// 定数バッファへ初期行列を転送
	TransferMatrix();
}

void WorldTransform::CreateConstBuffer()
{
	auto* resourceManager = Object3dManager::GetInstance()->GetDxManager()->GetResourceManager();
	// MVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	bufferHandle_ = resourceManager->CreateUploadBuffer(sizeof(TransformationMatrix), L"WorldTransformBuffer");
	// 書き込むためのアドレスを取得
	constMap = reinterpret_cast<TransformationMatrix*>(resourceManager->Map(bufferHandle_));
	// 単位行列を書き込んでおく
	constMap->WVP = MakeIdentity4x4();
	constMap->World = MakeIdentity4x4();
	constMap->WorldInverseTranspose = MakeIdentity4x4();
	
}

void WorldTransform::TransferMatrix()
{
	// スケール、回転、平行移動を合成して行列を計算する
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 親が存在する場合、親のワールド行列を掛け合わせる
	if (parent_) {
		matWorld_ *= parent_->matWorld_;
	}

	// ワールド行列を定数バッファに転送
	if (constMap != nullptr) {
		constMap->World = matWorld_; // 定数バッファに行列をコピー
		//constMap->WorldInverseTranspose = Inverse(matWorld_);
		constMap->WorldInverseTranspose = Transpose(Inverse(matWorld_));

		auto* camera = CameraManager::GetInstance()->GetActiveCamera();
		if (camera) {
			Matrix4x4 viewProj = CameraManager::GetInstance()->GetActiveCamera()->GetViewProjectionMatrix(); // あなたの実装に合わせて
			constMap->WVP = matWorld_ * viewProj; // row-vector版: W * V * P
		}
	}
}

void WorldTransform::BindToShader(ID3D12GraphicsCommandList* cmd, int32_t index) const
{
	auto* resourceManager = Object3dManager::GetInstance()->GetDxManager()->GetResourceManager();

	cmd->SetGraphicsRootConstantBufferView(index, resourceManager->GetGPUVirtualAddress(bufferHandle_));
}

#ifdef _DEBUG
void WorldTransform::DebugGui()
{
	if (ImGui::Button("ResetRotate")) {
		rotation_ = Identity();
	}
	ImGui::SameLine();
	if (ImGui::Button("ResetScale")) {
		scale_ = Vector3(1.0f, 1.0f, 1.0f);
	}
	ImGui::SameLine();
	if (ImGui::Button("ResetTranslate")) {
		translation_ = Vector3(0.0f, 0.0f, 0.0f);
	}

	ImGui::DragFloat3("translate", &translation_.x, 0.01f);
	ImGui::DragFloat3("scale", &scale_.x, 0.01f);
	Vector3 rotate = { 0.0f, 0.0f, 0.0f };
	ImGui::DragFloat3("rotation", &rotate.x, 0.1f);
	rotation_ = (rotation_ * Normalize(EulerDegree(rotate)));
}
#endif // _DEBUG

Vector3 WorldTransform::GetWorldPos()
{
	worldPos_.x = matWorld_.m[3][0];
	worldPos_.y = matWorld_.m[3][1];
	worldPos_.z = matWorld_.m[3][2];

	return worldPos_;
}
