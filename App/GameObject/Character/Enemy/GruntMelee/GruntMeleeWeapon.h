#pragma once
#include "3d/Object/Object3d.h"

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
};
