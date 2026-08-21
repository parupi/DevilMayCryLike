#include "WorldTransform.h"
#include "Math/MathUtils.h"
#include "Graphics/Device/DirectXManager.h"
#include "World3D/Object/Object3dManager.h"
#include "World3D/Camera/BaseCamera.h"
#ifdef _DEBUG
#include <imgui.h>
#endif // IMGUI
#include "Camera/CameraManager.h"
#include <cstring>

WorldTransform::~WorldTransform() {
	//// GPUリソースの解放
	//if (constBuffer_) {
	//	constBuffer_->Unmap(0, nullptr);
	//	constBuffer_.Reset();
	//}

#ifdef _DEBUG
	Logger::Log("WorldTransform resources released.\n");
#endif
}

void WorldTransform::Initialize() {
	// ワールド行列の初期化
	matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

	// 定数バッファ生成
	CreateConstBuffer();

	// 定数バッファへ初期行列を転送
	TransferMatrix(CameraManager::GetInstance().GetCurrentCamera());
}

void WorldTransform::CreateConstBuffer() {
	auto* resourceManager = Object3dManager::GetInstance().GetDxManager()->GetResourceManager();
	// MVP用のリソースを作る。Matrix4x4 1つ分のサイズを用意する
	bufferHandle_ = resourceManager->CreateUploadBuffer(sizeof(TransformationMatrix), L"WorldTransformBuffer");
	// 書き込むためのアドレスを取得
	constMap = reinterpret_cast<TransformationMatrix*>(resourceManager->Map(bufferHandle_));
	// 単位行列を書き込んでおく
	constMap->WVP = MakeIdentity4x4();
	constMap->World = MakeIdentity4x4();
	constMap->WorldInverseTranspose = MakeIdentity4x4();

}

void WorldTransform::TransferMatrix(BaseCamera* camera) {
	// ワールド行列の材料（SRT・ボーン追従・親）をまとめて前フレームと突き合わせる。
	// 動かないオブジェクトでは行列の合成も逆行列も丸ごと省ける。
	// SRTは参照で公開していてどこからでも書き換わるので、フラグではなく値の比較で見る
	TransformSource source{};
	source.scale = scale_;
	source.rotation = rotation_;
	source.translation = translation_;
	source.hasAttach = hasAttachMatrix_ ? 1u : 0u;
	if (hasAttachMatrix_) {
		source.attach = attachMatrix_;
	}
	if (parent_) {
		source.parentWorld = parent_->matWorld_;
	}

	const bool dirty = !sourceValid_ || std::memcmp(&source, &cachedSource_, sizeof(TransformSource)) != 0;

	if (dirty) {
		// スケール、回転、平行移動を合成して行列を計算する
		matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

		// ボーン追従の行列があればローカルの直後に挟む（行ベクトル規約なので 子 * 親 の順）
		if (hasAttachMatrix_) {
			matWorld_ *= attachMatrix_;
		}

		// 親が存在する場合、親のワールド行列を掛け合わせる
		if (parent_) {
			matWorld_ *= parent_->matWorld_;
		}

		// 逆転置行列は4x4の逆行列を解くので特に重い。ワールド行列が変わったときだけ求める
		cachedWorldInverseTranspose_ = Transpose(Inverse(matWorld_));

		cachedSource_ = source;
		sourceValid_ = true;

		if (constMap != nullptr) {
			constMap->World = matWorld_;
			constMap->WorldInverseTranspose = cachedWorldInverseTranspose_;
		}
	}

	// WVPはカメラが動くたびに変わるので毎フレーム更新する。
	// 渡されたカメラをそのまま使うこと。ここで CameraManager から引き直すと、
	// オブジェクトの数だけ名前でのmap検索が走る
	if (constMap != nullptr && camera) {
		constMap->WVP = matWorld_ * camera->GetViewProjectionMatrix();
	}
}

void WorldTransform::BindToShader(ID3D12GraphicsCommandList* cmd, int32_t index) const {
	auto* resourceManager = Object3dManager::GetInstance().GetDxManager()->GetResourceManager();

	cmd->SetGraphicsRootConstantBufferView(index, resourceManager->GetGPUVirtualAddress(bufferHandle_));
}

#ifdef _DEBUG
void WorldTransform::DebugGui() {
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

Vector3 WorldTransform::GetWorldPos() const {
	Vector3 worldPos;
	worldPos.x = matWorld_.m[3][0];
	worldPos.y = matWorld_.m[3][1];
	worldPos.z = matWorld_.m[3][2];

	return worldPos;
}

Vector3 WorldTransform::GetWorldScale() const {
	Vector3 scale{};

	scale.x = Length(Vector3{
		matWorld_.m[0][0],
		matWorld_.m[0][1],
		matWorld_.m[0][2]
		});

	scale.y = Length(Vector3{
		matWorld_.m[1][0],
		matWorld_.m[1][1],
		matWorld_.m[1][2]
		});

	scale.z = Length(Vector3{
		matWorld_.m[2][0],
		matWorld_.m[2][1],
		matWorld_.m[2][2]
		});

	return scale;
}

Vector3 WorldTransform::GetForward() const {
	// ワールド行列の3列目
	Vector3 forward{
		matWorld_.m[2][0],
		matWorld_.m[2][1],
		matWorld_.m[2][2]
	};

	// 正規化して返す
	return Normalize(forward);
}
