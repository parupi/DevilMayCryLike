#include "GruntStatePatrol.h"
#include "GameObject/Character/Enemy/Enemy.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"
#include "GameObject/Character/Enemy/Component/EnemySensorComponent.h"

GruntStatePatrol::GruntStatePatrol(EnemySensorComponent* sensor)
    : sensor_(sensor)
{
}

void GruntStatePatrol::Enter(Enemy& enemy)
{
    enemy;
}

void GruntStatePatrol::Update(Enemy& enemy, float deltaTime)
{
    deltaTime;
    if (!enemy.GetOnGround()) {
        enemy.ChangeState(EnemyStateName::Air);
        return;
    }

    sensor_->Update(enemy);

    if (sensor_->IsPlayerDetected()) {
        enemy.ChangeState(GruntMeleeStateName::CombatIdle);
    }
}

void GruntStatePatrol::Exit(Enemy& enemy)
{
    enemy;
}
