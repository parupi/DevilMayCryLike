#include "PlayerWeapon.h"
#include <Collider/AABBCollider.h>

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName)
{
}

void PlayerWeapon::Initialize()
{
	Object3d::Initialize();

	GetWorldTransform()->GetTranslation() = {0.75f, -1.3f, -0.3f};

	//GetRenderer("PlayerWeapon")->GetWorldTransform()->GetTranslation() = { 0.75f, -2.5f, -1.0f };
	GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, -30.0f, 90.0f });
	//GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {3.0f, 3.0f, 3.0f};

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMax = { 0.5f, 0.5f, 0.5f };

	Object3d::Update();
}

void PlayerWeapon::Update()
{
	//if (isAttack_) {
	//	//GetWorldTransform()->GetTranslation().y -= 50.0f * (1.0f / 60.0f);
	//} else {
	//	//GetWorldTransform()->GetTranslation().y = 8.0f;
	//}

	Object3d::Update();
}

void PlayerWeapon::Draw()
{
	Object3d::Draw();
}

void PlayerWeapon::DrawEffect()
{
}

void PlayerWeapon::OnCollisionEnter(BaseCollider* other)
{
}
void PlayerWeapon::OnCollisionStay(BaseCollider* other)
{
}
void PlayerWeapon::OnCollisionExit(BaseCollider* other)
{
}

#ifdef _DEBUG
void PlayerWeapon::DebugGui()
{
	Object3d::DebugGui();

}

#endif // _DEBUG

