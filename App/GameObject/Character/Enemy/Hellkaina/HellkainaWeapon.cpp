#include "HellkainaWeapon.h"

HellkainaWeapon::HellkainaWeapon(std::string objectName) : Object3d(objectName)
{
}

void HellkainaWeapon::Initialize()
{
	Object3d::Initialize();
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };

	Object3d::Update();
}

void HellkainaWeapon::Update()
{
	Object3d::Update();
}

void HellkainaWeapon::Draw()
{
	Object3d::Draw();
}

#ifdef _DEBUG
void HellkainaWeapon::DebugGui()
{
	Object3d::DebugGui();
}
#endif // DEBUG

void HellkainaWeapon::OnCollisionEnter(BaseCollider* other)
{
}

void HellkainaWeapon::OnCollisionStay(BaseCollider* other)
{
}

void HellkainaWeapon::OnCollisionExit(BaseCollider* other)
{
}
