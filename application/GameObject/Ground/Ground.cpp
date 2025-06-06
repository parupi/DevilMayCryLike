#include "Ground.h"

Ground::Ground()
{

}

void Ground::Initialize()
{
	Object3d::Initialize();

	GetWorldTransform()->GetTranslation().y = -5.0f;
	GetWorldTransform()->GetScale() = { 60.0f, 1.0f, 100.0f };

	static_cast<Model*>(GetRenderer("Ground")->GetModel())->GetMaterials(0)->SetColor({ 0.5f, 0.5f, 0.5f, 1.0f });

	Object3d::Update();
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


