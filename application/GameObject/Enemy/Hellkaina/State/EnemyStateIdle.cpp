#include "EnemyStateIdle.h"
#include "GameObject/Enemy/Enemy.h"
// 初期化
//std::srand(static_cast<unsigned int>(std::time(nullptr)));

void EnemyStateIdle::Enter(Enemy& enemy)
{
}

void EnemyStateIdle::Update(Enemy& enemy)
{
	// ランダムの値で次が移動か攻撃かを決める
	int r = std::rand() % 100; // 0～99のランダムな整数

	if (r < 70) {
		enemy.ChangeState("Move");
		return;
	} else {
		//enemy.ChangeState("Attack");
		//return;
	}

	if (!enemy.GetOnGround()) {
		enemy.ChangeState("Air");
		return;
	}
}

void EnemyStateIdle::Exit(Enemy& enemy)
{
}
