#pragma once
#include "GameObject/Character/Enemy/Enemy.h"
#include "HellkainaWeapon.h"
#include "base/Particle/ParticleEmitter.h"

class Hellkaina : public Enemy
{
public:
    Hellkaina(std::string objectName);
    void Initialize() override;
    void Update(float deltaTime) override;

#ifdef _DEBUG
    void DebugGui() override;
#endif

    void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
    void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

private:
    HellkainaWeapon* weapon_ = nullptr;
    ParticleEmitter* emitter_ = nullptr;
};
