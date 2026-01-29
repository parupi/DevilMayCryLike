#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
#include <cstdlib> 
#include <ctime>   

namespace
{
    enum class LastAttack
    {
        None,
        AttackA,
        AttackB
    };
}

class EnemyStateIdle : public EnemyStateBase
{
public:
	EnemyStateIdle() = default;
	~EnemyStateIdle() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy, float deltaTime) override;
	void Exit(Enemy& enemy) override;
private:

};

