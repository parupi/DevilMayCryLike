#include "GruntMeleeWeapon.h"

GruntMeleeWeapon::GruntMeleeWeapon(std::string objectName) : Object3d(objectName)
{
}

void GruntMeleeWeapon::Initialize()
{
    Object3d::Initialize();
    GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.5f, 0.5f, 0.5f };
}

void GruntMeleeWeapon::Update(float deltaTime)
{
    Object3d::Update(deltaTime);
}

void GruntMeleeWeapon::Draw()
{
    Object3d::Draw();
}

#ifdef _DEBUG
void GruntMeleeWeapon::DebugGui()
{
    Object3d::DebugGui();
}
#endif

void GruntMeleeWeapon::OnCollisionEnter(BaseCollider* other) { other; }
void GruntMeleeWeapon::OnCollisionStay(BaseCollider* other)  { other; }
void GruntMeleeWeapon::OnCollisionExit(BaseCollider* other)  { other; }
