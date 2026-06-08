#pragma once
#include <Math/Vector3.h>

class Enemy;

/// <summary>
/// 移動実行コンポーネント。
/// ステートから呼ばれ、enemy の velocity に書き込む。
/// </summary>
class EnemyMovementComponent {
public:
    /// プレイヤー方向へ接近。stopDistance 以内では止まる。
    void MoveToward(Enemy& enemy, float speed, float stopDistance = 2.5f);

    /// プレイヤーから離れる後退。
    void MoveAway(Enemy& enemy, float speed);

    /// プレイヤーを軸に左右へ移動。direction: +1 右、-1 左。
    void MoveSideways(Enemy& enemy, float speed, float direction);

    /// 速度をゼロにする。
    void Stop(Enemy& enemy);
};
