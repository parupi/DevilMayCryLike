#pragma once
#include "GameObject/Enemy/EnemyStateBase.h"
#include "3d/Object/Object3d.h"
class HellkainaStateKnockBack : public EnemyStateBase
{
public:
	HellkainaStateKnockBack() = default;
	~HellkainaStateKnockBack() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy) override;
	void Exit(Enemy& enemy) override;
private:
	TimeData stateTime_;
};

