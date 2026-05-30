#pragma once
#include "GameObject/Character/Enemy/BaseState/EnemyStateBase.h"
#include "3d/Object/Object3d.h"
#include "GameObject/Character/Enemy/Hellkaina/HellkainaWeapon.h"

// 武器モーション付き攻撃の汎用ステート。
// コンストラクタで武器ポインタとモーションパラメータを受け取るため
// static_cast なしで任意の武器アニメーションを実装できる。
struct WeaponMotionParams {
    float duration  = 0.25f;
    float moveSpeed = 0.0f;
    std::vector<Vector3> translate;
    std::vector<Vector3> rotate;
};

class HellkainaWeaponAttackState : public EnemyStateBase
{
public:
    HellkainaWeaponAttackState(HellkainaWeapon* weapon, WeaponMotionParams params);
    ~HellkainaWeaponAttackState() override = default;

    void Enter(Enemy& enemy) override;
    void Update(Enemy& enemy, float deltaTime) override;
    void Exit(Enemy& enemy) override;

private:
    HellkainaWeapon* weapon_;
    WeaponMotionParams params_;
    float timer_ = 0.0f;
};
