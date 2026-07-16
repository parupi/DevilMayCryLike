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
	// レベルエディタ(file_name)で指定されたモデルを未読み込みなら読み込む
	ModelManager::GetInstance().LoadModel(modelName_);

	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, modelName_));

	AddRenderer(RendererManager::GetInstance().FindRender(name_));

	GetCollider(name_)->category_ = CollisionCategory::Ground;

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


#ifdef _DEBUG
void Ground::DebugGui()
{
	ImGui::Begin("Ground");
	Object3d::DebugGui();
	ImGui::End();
}
#endif // _DEBUG


