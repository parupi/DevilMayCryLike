#include "PlayerWeapon.h"
#include <3d/Collider/AABBCollider.h>

PlayerWeapon::PlayerWeapon(std::string objectName) : Object3d(objectName)
{
}

void PlayerWeapon::Initialize()
{
	Object3d::Initialize();

	//GetWorldTransform()->GetTranslation() = {0.75f, -1.3f, -0.3f};

	GetRenderer("PlayerWeapon")->GetWorldTransform()->GetTranslation() = { -1.5f, -0.15f, 0.0f };
	//GetWorldTransform()->GetRotation() = EulerDegree({ 0.0f, -30.0f, 90.0f });
	//GetRenderer("PlayerWeapon")->GetWorldTransform()->GetScale() = {3.0f, 3.0f, 3.0f};

	GetCollider("WeaponCollider")->category_ = CollisionCategory::PlayerWeapon;
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMax = { 0.5f, 0.5f, 0.5f };
	static_cast<AABBCollider*>(GetCollider("WeaponCollider"))->GetColliderData().offsetMin = { -0.5f, -0.5f, -0.5f };

	Object3d::Update();
}

void PlayerWeapon::Update()
{
	ImGui::Begin("Weapon");
	ImGui::DragFloat3("translate", &GetRenderer("PlayerWeapon")->GetWorldTransform()->GetTranslation().x, 0.01f);
	ImGui::End();

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

