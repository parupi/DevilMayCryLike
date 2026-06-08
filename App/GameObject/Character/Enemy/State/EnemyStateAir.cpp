#include "EnemyStateAir.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"

void EnemyStateAir::Enter(Enemy& enemy)
{
	enemy.SetAcceleration({ 0.0f, -9.8f, 0.0f });
}

void EnemyStateAir::Update(Enemy& enemy, float deltaTime)
{
	deltaTime;
	if (enemy.GetOnGround()) {
		enemy.ChangeState(EnemyStateName::Move);
	}
}

void EnemyStateAir::Exit(Enemy& enemy)
{
	enemy.SetAcceleration({ 0.0f, 0.0f, 0.0f });
}
