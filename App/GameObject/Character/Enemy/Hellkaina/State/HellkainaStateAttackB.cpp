#include "HellkainaStateAttackB.h"
#include "GameObject/Character/Enemy/Hellkaina/Hellkaina.h"
#include "GameObject/Character/Player/Player.h"

void HellkainaStateAttackB::Enter(Enemy& enemy)
{
	enemy.SetIsAttack(true);

	timer_ = 0.0f;
	translate_.resize(4);
	translate_ =
	{
		{ -0.75f, 0.75f, -0.75f },
		{ -0.75f, 0.75f, -0.75f },
		{  0.0f,  0.35f, -0.85f },
		{  0.25f, 0.0f,  -1.0f }
	};

	rotate_.resize(4);
	rotate_ =
	{
		{ 0.0f, 0.0f, 30.0f },
		{ 0.0f, 0.0f, 30.0f },
		{ -45.0f, 0.0f, 30.0f },
		{ -100.0f, 0.0f, 30.0f }
	};
}

void HellkainaStateAttackB::Update(Enemy& enemy)
{
	timer_ += DeltaTime::GetDeltaTime();
	float t = timer_ / time_;

	Vector3 currentTranslate = CatmullRomSpline(translate_, t);
	Vector3 currentRotate = CatmullRomSpline(rotate_, t);
	Hellkaina& hellkaina = static_cast<Hellkaina&>(enemy);
	hellkaina.GetWeapon()->GetWorldTransform()->GetTranslation() = currentTranslate;
	hellkaina.GetWeapon()->GetRenderer(hellkaina.GetWeapon()->name_)->GetWorldTransform()->GetRotation() = EulerDegree(currentRotate);

	Vector3 dir = Normalize(enemy.GetPlayer()->GetWorldTransform()->GetTranslation() - enemy.GetWorldTransform()->GetTranslation());
	dir.y = 0.0f;
	Vector3 velocity = dir * 16.0f;
	enemy.GetWorldTransform()->GetTranslation() += velocity * DeltaTime::GetDeltaTime();

	if (timer_ >= time_) {
		enemy.ChangeState("Idle");
	}
}

void HellkainaStateAttackB::Exit(Enemy& enemy)
{
	enemy.SetIsAttack(false);
}
