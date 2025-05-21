#include "Object3d.h"
#include "Object3dManager.h"
#include "TextureManager.h"
//#include "imgui.h"
#include <WorldTransform.h>
#include <numbers>
//#include <Skeleton.h>
//#include <Animator.h>
#include "Model/ModelManager.h"
Object3d::~Object3d()
{
}

void Object3d::Initialize(const std::string& fileName)
{

	objectManager_ = Object3dManager::GetInstance();

	// モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(fileName);
	if (!model_) {
		assert(false && "Model not found.");
	}

	transform_ = std::make_unique<WorldTransform>();
	transform_->Initialize();

	CreateCameraResource();
}

void Object3d::AnimationUpdate()
{
	//if (model_->GetModelData().isAnimation) {
	//model_->Update();
	//}
}

void Object3d::Update()
{
	transform_->TransferMatrix();

	model_->Update();

	camera_ = objectManager_->GetDefaultCamera();
	cameraData_->worldPosition = camera_->GetTranslate();

	Matrix4x4 worldViewProjectionMatrix;
	if (camera_) {
		const Matrix4x4& viewProjectionMatrix = camera_->GetViewProjectionMatrix();
		worldViewProjectionMatrix = transform_->GetMatWorld() * viewProjectionMatrix;
	} else {
		worldViewProjectionMatrix = transform_->GetMatWorld();
	}

	transform_->SetMapWVP(worldViewProjectionMatrix);
	transform_->SetMapWorld(transform_->GetMatWorld());
}

void Object3d::Draw()
{
	// cameraの場所を指定
	objectManager_->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(3, cameraResource_->GetGPUVirtualAddress());
	// wvp用のCBufferの場所を設定
	objectManager_->GetDxManager()->GetCommandList()->SetGraphicsRootConstantBufferView(1, transform_->GetConstBuffer()->GetGPUVirtualAddress());
	// 3Dモデルが割り当てられていれば描画する
	if (model_) {
		model_->Draw();
	}
}


#ifdef _DEBUG
void Object3d::DebugGui()
{
	model_->DebugGui(this);

	transform_->DebugGui();
}
#endif // _DEBUG

void Object3d::CreateCameraResource()
{
	// カメラ用のリソースを作る
	cameraResource_ = objectManager_->GetDxManager()->CreateBufferResource(sizeof(CameraForGPU));
	// 書き込むためのアドレスを取得
	cameraResource_->Map(0, nullptr, reinterpret_cast<void**>(&cameraData_));
	// 初期値を入れる
	cameraData_->worldPosition = { 1.0f, 1.0f, 1.0f };
}

void Object3d::SetModel(const std::string& filePath)
{
	// モデルを検索してセットする
	model_ = ModelManager::GetInstance()->FindModel(filePath);
}
