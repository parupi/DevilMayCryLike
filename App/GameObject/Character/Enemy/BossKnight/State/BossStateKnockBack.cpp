#include "BossStateKnockBack.h"
#include "GameObject/Character/Enemy/EnemyStateNames.h"

const char* BossStateKnockBack::NextState() const {
	return BossStateName::Rush;
}
