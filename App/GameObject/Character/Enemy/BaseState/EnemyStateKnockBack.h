#pragma once
#include "EnemyStateBase.h"

// KnockBack 系ステートの基底クラス。
// Enter() は EnemyStateBase と同じシグネチャ。
// DamageInfo は Enter() 内で enemy.GetPendingDamageInfo() から取得する。
class EnemyStateKnockBack : public EnemyStateBase {
};
