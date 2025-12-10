#pragma once
#include "EnemyStateBase.h"
#include <GameObject/Character/CharacterStructs.h>

class Enemy;
class EnemyStateKnockBack : public EnemyStateBase {
public:
    virtual void Enter(const DamageInfo& info, Enemy& enemy) = 0;
};

