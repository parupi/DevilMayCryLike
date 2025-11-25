#include "HellkainaStateKnockBack.h"
#include <base/utility/DeltaTime.h>
#include "GameObject/Enemy/Enemy.h"

void HellkainaStateKnockBack::Enter(Enemy& enemy)
{
	stateTime_.current = 0.0f;
	stateTime_.max = 0.3f;
	enemy.SetAcceleration({ 0.0f, 0.0f, 0.0f });
	enemy.SetVelocity({ 0.0f, 0.0f, 0.0f });
}

void HellkainaStateKnockBack::Update(Enemy& enemy)
{
	stateTime_.current += (1.0f / stateTime_.max) * DeltaTime::GetDeltaTime();



	if (stateTime_.current >= 1.0f) {
		enemy.ChangeState("Idle");
		return;
	}
}

void HellkainaStateKnockBack::Exit(Enemy& enemy)
{

}
