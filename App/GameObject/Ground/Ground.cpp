#include "Ground.h"
#include <World3D/Object/Renderer/RendererManager.h>
#include <memory>
#include "World3D/Object/Renderer/ModelRenderer.h"

Ground::Ground(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();
}

void Ground::Initialize()
{
	// レンダラーの生成
	RendererManager::GetInstance().AddRenderer(std::make_unique<ModelRenderer>(name_, "Cube"));

	AddRenderer(RendererManager::GetInstance().FindRender(name_));

	GetCollider(name_)->category_ = CollisionCategory::Ground;
	// uvサイズをオブジェクトの大きさに合わせる
	GetRenderer(name_)->GetModel()->GetMaterials()[1]->SetEnableTextureDensity(true);
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


