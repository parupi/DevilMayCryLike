#pragma once
#include <string>
#include <memory>

class Enemy;
class EnemyStateBase
{
public:
	virtual ~EnemyStateBase() = default;
	virtual void Enter(Enemy& enemy) = 0;
	virtual void Update(Enemy& enemy) = 0;
	virtual void Exit(Enemy& enemy) = 0;
};

