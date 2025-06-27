#include "PlayerStateAttack1.h"
#include "Vector3.h"
#include <utility/DeltaTime.h>

PlayerStateAttack1::PlayerStateAttack1(std::string attackName) : PlayerStateAttackBase(attackName)
{

}

void PlayerStateAttack1::Enter(Player& player)
{
	attackPhase_ = AttackPhase::Startup;
	stateTime_.current = 0.0f;
	stateTime_.max = raiseDuration;

	startRot_ = {};              // 現在の腕の角度を初期値とする
	targetRot_.x = -157.0f;      // 振りかぶり角度

	attackData_.pointCount = gv->GetValueRef<int32_t>(name, "PointCount");

	for (int32_t i = 0; i < attackData_.pointCount; i++) {
		attackData_.controlPoints.push_back(gv->GetValueRef<Vector3>(name, "ControlPoint_" + std::to_string(i)));
	}
}

void PlayerStateAttack1::Update(Player& player)
{
	stateTime_.current += DeltaTime::GetDeltaTime();
	float t = std::clamp(stateTime_.current / stateTime_.max, 0.0f, 1.0f);





	Vector3 pos = CatmullRom(
		attackData_.controlPoints[0],
		attackData_.controlPoints[1],
		attackData_.controlPoints[2],
		attackData_.controlPoints[3],
		t
	);

	player.GetWeapon()->GetWorldTransform()->GetTranslation() = pos;

	if (t >= 1.0f) {
		player.ChangeState("Idle");
	}
}

void PlayerStateAttack1::Exit(Player& player)
{
	Vector3 newRot{};
	newRot.x = 0.0f;

	player.GetWeapon()->SetIsAttack(false);
}
