#pragma once
#include "GameObject/Enemy/EnemyStateBase.h"
#include <cstdlib> 
#include <ctime>   

class EnemyStateIdle : public EnemyStateBase
{
public:
	EnemyStateIdle() = default;
	~EnemyStateIdle() override = default;
	void Enter(Enemy& enemy) override;
	void Update(Enemy& enemy) override;
	void Exit(Enemy& enemy) override;
private:


};

