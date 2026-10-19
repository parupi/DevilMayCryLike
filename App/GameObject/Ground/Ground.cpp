#include "Ground.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <memory>
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "World3D/Object/Model/ModelManager.h"

Ground::Ground(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();
}

void Ground::Initialize()
{
	// モデル未指定なら Cube。ここで確定させておけば保存にもそのまま出る
	if (GetModelName().empty()) {
		SetModelName("Cube");
	}

	// ステージデータで指定されたモデルを未読み込みなら読み込む
	ModelManager::GetInstance().LoadModel(GetModelName());

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, GetModelName()));

	AddRenderer(RendererManager::GetInstance().FindRender(name_));
	// 壁や床は環境オブジェクト。洞窟の「高さによる暗化」はこれが立っているものだけに掛かる
	GetRenderer(name_)->SetEnvironmentSurface(true);
	// 動かない環境モデルなので、同じモデルの床・壁とまとめて1回で描く（StaticInstanceBatcher）
	if (auto* modelRenderer = dynamic_cast<ModelRenderer*>(GetRenderer(name_))) {
		modelRenderer->SetInstanced(true);
	}

	// コライダーはステージデータで付けた場合のみ存在する。
	// エディタで作った直後は無いので、付いているものだけ Ground カテゴリにする
	for (BaseCollider* collider : GetColliders()) {
		collider->category_ = CollisionCategory::Ground;
	}

	// uvサイズをオブジェクトの大きさに合わせる(モデルによってはマテリアルが1つしか無いので安全にアクセスする)
	std::vector<Material*> materials = GetRenderer(name_)->GetModel()->GetMaterials();
	if (!materials.empty()) {
		materials[materials.size() > 1 ? 1 : 0]->SetEnableTextureDensity(true);
	}
}

void Ground::Update(float deltaTime)
{
	Object3d::Update(deltaTime);
}

void Ground::Draw()
{
	Object3d::Draw();
}




