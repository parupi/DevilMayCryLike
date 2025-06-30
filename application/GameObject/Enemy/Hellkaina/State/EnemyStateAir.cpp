#include "EnemyStateAir.h"
#include "GameObject/Enemy/Enemy.h"
void EnemyStateAir::Enter(Enemy& enemy)
{
	enemy.SetAcceleration({ 0.0f, -9.8f, 0.0f });
}

void EnemyStateAir::Update(Enemy& enemy)
{
	if (enemy.GetOnGround()) {
		enemy.ChangeState("Move");
	}
}

void EnemyStateAir::Exit(Enemy& enemy)
{
	enemy.SetAcceleration({ 0.0f, 0.0f, 0.0f });
}
