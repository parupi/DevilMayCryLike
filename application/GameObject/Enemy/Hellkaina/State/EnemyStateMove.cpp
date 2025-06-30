#include "EnemyStateMove.h"
#include "GameObject/Enemy/Enemy.h"
#include "GameObject/Player/Player.h"
void EnemyStateMove::Enter(Enemy& enemy)
{
    stateTime_.max = 2.0f;
}

void EnemyStateMove::Update(Enemy& enemy)
{
    stateTime_.current += (1.0f / stateTime_.max) * DeltaTime::GetDeltaTime();

    // 敵の現在の速度を取得
    Vector3 currentVelocity = enemy.GetVelocity();
    // プレイヤーの位置を取得
    Vector3 playerPos = enemy.GetPlayer()->GetWorldTransform()->GetTranslation();
    // 敵の現在位置を取得
    Vector3 enemyPos = enemy.GetWorldTransform()->GetTranslation();
    // プレイヤーへの方向ベクトル
    Vector3 toPlayer = playerPos - enemyPos;

    // 長さ（距離）が十分にあるときのみ移動
    if (Length(toPlayer) > 10.0f) {
        Vector3 direction = Normalize(toPlayer);
        float speed = 0.5f;

        // 新しいX,Z方向の速度を計算
        Vector3 newVelocity = direction * speed;

        // Yは元の速度を使う
        newVelocity.y = currentVelocity.y;

        enemy.SetVelocity(newVelocity);
    } else {
        Vector3 newVelocity = { 0.0f, 0.0f, 0.0f };

        // Yは元の速度を使う
        newVelocity.y = currentVelocity.y;

        enemy.SetVelocity(newVelocity);
    }

    if (stateTime_.current >= 1.0f) {
        enemy.ChangeState("Idle");
    }

	if (!enemy.GetOnGround()) {
		enemy.ChangeState("Air");
        return;
	}
}

void EnemyStateMove::Exit(Enemy& enemy)
{
}
