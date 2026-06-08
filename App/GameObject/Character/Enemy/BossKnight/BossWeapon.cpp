#include "BossWeapon.h"

BossWeapon::BossWeapon(std::string objectName) : Object3d(objectName) {}

void BossWeapon::Initialize()
{
    Object3d::Initialize();
    GetRenderer(name_)->GetWorldTransform()->GetScale() = { 0.7f, 0.7f, 0.7f };
}

void BossWeapon::Update(float deltaTime) { Object3d::Update(deltaTime); }
void BossWeapon::Draw() { Object3d::Draw(); }

#ifdef _DEBUG
void BossWeapon::DebugGui() { Object3d::DebugGui(); }
#endif
