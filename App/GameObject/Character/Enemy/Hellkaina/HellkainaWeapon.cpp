#include "HellkainaWeapon.h"

HellkainaWeapon::HellkainaWeapon(std::string objectName) : Object3d(objectName)
{
}

void HellkainaWeapon::Initialize()
{
	Object3d::Initialize();
	GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };
}

void HellkainaWeapon::Update(float deltaTime)
{
	Object3d::Update(deltaTime);
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
