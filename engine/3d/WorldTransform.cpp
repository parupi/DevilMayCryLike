#include <3d/WorldTransform.h>
#include <math/function.h>
#include <base/DirectXManager.h>
#include <3d/Object/Object3dManager.h>

WorldTransform::~WorldTransform()
{
	// GPUリソースの解放
	if (constBuffer_) {
		constBuffer_->Unmap(0, nullptr);
		constBuffer_.Reset();
	}

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
	// MVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	constBuffer_ = Object3dManager::GetInstance()->GetDxManager()->CreateBufferResource(sizeof(TransformationMatrix));
	// 書き込むためのアドレスを取得
	constBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&constMap));
	// 単位行列を書き込んでおく
	constMap->WVP = MakeIdentity4x4();
	constMap->World = MakeIdentity4x4();
	constMap->WorldInverseTranspose = MakeIdentity4x4();

	// ログ出力
	Logger::LogBufferCreation("WorldTransform", constBuffer_.Get(), sizeof(TransformationMatrix));
}

void WorldTransform::TransferMatrix()
{
	// スケール、回転、平行移動を合成して行列を計算する
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 親が存在する場合、親のワールド行列を掛け合わせる
	if (parent_) {
		Matrix4x4 parentMatrix = parent_->matWorld_;
		matWorld_ = matWorld_ * parentMatrix; // 親の行列と自身の行列を合成
	}

	// ワールド行列を定数バッファに転送
	if (constMap != nullptr) {
		constMap->World = matWorld_; // 定数バッファに行列をコピー
		constMap->WorldInverseTranspose = Inverse(matWorld_);
	}
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
