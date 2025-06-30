#include "Ground.h"

Ground::Ground(std::string objectName) : Object3d(objectName)
{
	Object3d::Initialize();
}

void Ground::Initialize()
{


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
	ImGui::Begin("Ground");
	Object3d::DebugGui();
	ImGui::End();
}
#endif // _DEBUG


