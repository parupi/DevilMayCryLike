#pragma once
#include "World3D/Object/Object3d.h"
#include "GameObject/Character/CharacterStructs.h"

class GruntMeleeWeapon : public Object3d
{
public:
    GruntMeleeWeapon(std::string objectName);
    ~GruntMeleeWeapon() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw() override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter(BaseCollider* other) override;
    void OnCollisionStay(BaseCollider* other) override;
    void OnCollisionExit(BaseCollider* other) override;

    /// <summary>この剣に当たったときのダメージとリアクション（Player::OnCollisionEnter が読む）</summary>
    DamageInfo GetDamageInfo() const;

    // 1回当たったときのダメージ。プレイヤーの最大HP(100)に対して9発で倒れる
    static constexpr float kDamage = 12.0f;
};
