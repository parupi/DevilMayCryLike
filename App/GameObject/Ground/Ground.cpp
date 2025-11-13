#include "Ground.h"
#include <3d/Object/Renderer/RendererManager.h>
#include <memory>
#include "3d/Object/Renderer/ModelRenderer.h"

Ground::Ground(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();
}

void Ground::Initialize()
{
	
	// レンダラーの生成
	RendererManager::GetInstance()->AddRenderer(std::make_unique<ModelRenderer>(name_, "Cube"));

	AddRenderer(RendererManager::GetInstance()->FindRender(name_));

	GetCollider(name_)->category_ = CollisionCategory::Ground;
}

void Ground::Update()
{
	Object3d::Update();
}

void Ground::Draw()
{
	Object3d::Draw();
}


#ifdef _DEBUG
void Ground::DebugGui()
{
	//ImGui::Begin("Ground");
	//Object3d::DebugGui();
	//ImGui::End();
}
#endif // _DEBUG


