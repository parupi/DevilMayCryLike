#pragma once
#include <memory>
#include "GameObject/Character/Enemy/Enemy.h"
#include "HellkainaWeapon.h"

class Hellkaina : public Enemy
{
public:
    Hellkaina(std::string objectName);
    // 初期化
    void Initialize() override;
    // 更新
    void Update(float deltaTime) override;

#ifdef _DEBUG
    void DebugGui() override;
#endif // DEBUG

     // 衝突系
    void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

    HellkainaWeapon* GetWeapon() { return weapon_; }
private:
    HellkainaWeapon* weapon_; ///< 武器クラス
};

