#pragma once
#include "3d/Object/Object3d.h"
#include "base/Particle/ParticleEmitter.h"
#include "EnemyDamageEffect.h"
#include "EnemyStateBase.h"
class Player;
class Enemy : public Object3d
{
public:
    Enemy(std::string objectName);
    virtual ~Enemy() override = default;

    // 初期化と更新は virtual にする
    virtual void Initialize() override;
    virtual void Update() override;
    virtual void Draw() override;
    virtual void DrawEffect();

#ifdef _DEBUG
    virtual void DebugGui() override;
#endif // _DEBUG

    // 衝突系
    virtual void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
    virtual void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
    virtual void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

    // 状態切り替え（共通）
    void ChangeState(const std::string& stateName);

    // Getter / Setter（共通）
    bool GetOnGround() const { return onGround_; }
    Player* GetPlayer() { return player_; }
    void SetPlayer(Player* player) { player_ = player; }

    Vector3 GetVelocity() const { return velocity_; }
    void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

    Vector3 GetAcceleration() const { return acceleration_; }
    void SetAcceleration(const Vector3& acceleration) { acceleration_ = acceleration; }

    float GetHp() const { return hp_; }
    void SetHp(float hp) { hp_ = hp; }

protected:
    std::unordered_map<std::string, std::unique_ptr<EnemyStateBase>> states_;
    EnemyStateBase* currentState_ = nullptr;

    Player* player_ = nullptr;

    Vector3 velocity_{};
    Vector3 acceleration_{ 0.0f, 0.0f, 0.0f };

    float hp_ = 3;
    bool onGround_ = false;
};
