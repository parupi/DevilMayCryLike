#include "HellkainaStateSideMove.h"
#include "GameObject/Character/Enemy/Hellkaina/Hellkaina.h"
#include "GameObject/Character/Player/Player.h"
#include <cstdlib>

void HellkainaStateSideMove::Enter(Enemy& enemy)
{
    timer_ = 0.0f;

    // 左右どちらかを選ぶ（-1 or +1）
    sideDir_ = (std::rand() % 2 == 0) ? -1.0f : 1.0f;
}

void HellkainaStateSideMove::Update(Enemy& enemy)
{
    timer_ += DeltaTime::GetDeltaTime();

    Player* player = enemy.GetPlayer();
    if (!player) {
        return;
    }

    // -----------------------------
    // プレイヤー方向ベクトル
    // -----------------------------
    Vector3 enemyPos = enemy.GetWorldTransform()->GetTranslation();
    Vector3 playerPos = player->GetWorldTransform()->GetTranslation();

    Vector3 toPlayer = playerPos - enemyPos;
    toPlayer.y = 0.0f; // 水平移動だけにする
    Normalize(toPlayer);

    // -----------------------------
    // 横方向ベクトル（右方向）
    // Y軸上向き前提の外積
    // -----------------------------
    Vector3 up(0.0f, 1.0f, 0.0f);
    Vector3 sideDir = Cross(up, toPlayer);
    Normalize(sideDir);

    // -----------------------------
    // 横移動
    // -----------------------------
    enemy.SetVelocity(sideDir * sideDir_ * moveSpeed_);

    // -----------------------------
    // 時間経過で終了（次は Idle / Move に任せる）
    // -----------------------------
    if (timer_ >= time_) {
        enemy.ChangeState("Idle");
        return;
    }
}

void HellkainaStateSideMove::Exit(Enemy& enemy)
{
    // 横移動の影響を消す
    enemy.SetVelocity({ 0.0f, 0.0f, 0.0f });
}
