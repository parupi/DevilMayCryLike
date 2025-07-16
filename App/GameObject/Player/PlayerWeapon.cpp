#include "PlayerWeapon.h"
#include <3d/Collider/AABBCollider.h>

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName)
{
}

void PlayerWeapon::Initialize()
{
	Object3d::Initialize();

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {1.0f, 0.25f, 1.0f };

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMax = { 0.5f, 0.5f, 0.5f };
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMin = { -0.5f, -0.5f, -0.5f };

	Object3d::Update();
}

void PlayerWeapon::Update()
{


	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().isActive = isAttack_;
	
	
	Object3d::Update();
}

void PlayerWeapon::Draw()
{
	if (isAttack_) {
		Object3d::Draw();
	}
}

void PlayerWeapon::DrawEffect()
{
}

void PlayerWeapon::OnCollisionEnter(BaseCollider* other)
{
	other;
}
void PlayerWeapon::OnCollisionStay(BaseCollider* other)
{
	other;
}
void PlayerWeapon::OnCollisionExit(BaseCollider* other)
{
	other;
}

#ifdef _DEBUG
void PlayerWeapon::DebugGui()
{
	Object3d::DebugGui();

}

#endif // _DEBUG

