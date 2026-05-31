#pragma once
#include "3d/Object/Object3d.h"

class BossWeapon : public Object3d
{
public:
    BossWeapon(std::string objectName);
    ~BossWeapon() override = default;

    void Initialize() override;
    void Update(float deltaTime) override;
    void Draw() override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override {}
    void OnCollisionStay([[maybe_unused]] BaseCollider* other) override {}
    void OnCollisionExit([[maybe_unused]] BaseCollider* other) override {}
};
