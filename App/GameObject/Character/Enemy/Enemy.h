#pragma once
#include "World3D/Object/Object3d.h"
#include "BaseState/EnemyStateBase.h"
#include "GameObject/Effect/HitStop.h"
#include "GameObject/Character/CharacterStructs.h"
#include <GameObject/LockOn/LockOnTarget.h>

class Player;

/// <summary>
/// 敵キャラクターの基底クラス。
/// ステートマシン・物理演算・地形衝突・接地判定・死亡管理を担う。
/// 武器・エフェクト・被弾ロジックなどキャラクター固有の処理は派生クラスで実装する。
/// </summary>
class Enemy : public Object3d
{
public:
    Enemy(std::string objectName);
    virtual ~Enemy() override;

    /// <summary>
    /// 敵の初期化処理  
    /// 各種ステート・エフェクト・物理パラメータを設定する。
    /// </summary>
    virtual void Initialize() override;

    /// <summary>
    /// 敵の更新処理  
    /// 現在の状態（ステート）に応じた行動・移動・攻撃を行う。
    /// </summary>
    virtual void Update(float deltaTime) override;

    /// <summary>
    /// 敵の描画処理  
    /// モデルや外見をレンダリングする。
    /// </summary>
    virtual void Draw() override;

    /// <summary>
    /// 敵のエフェクト描画処理  
    /// パーティクルやダメージエフェクトなどを描画する。
    /// </summary>
    virtual void DrawEffect();

	void Spawn();

#ifdef _DEBUG
    /// <summary>
    /// デバッグ用GUI描画処理  
    /// 敵の内部状態（HP、速度、状態名など）を可視化する。
    /// </summary>
    virtual void DebugGui() override;
#endif // _DEBUG

    // ======================
    // 衝突処理
    // ======================

    /// <summary>
    /// 衝突開始時の処理  
    /// プレイヤー攻撃や地形との最初の接触時に呼ばれる。
    /// </summary>
    virtual void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

    /// <summary>
    /// 衝突中の処理  
    /// 接触中のフレームごとに呼ばれる。
    /// </summary>
    virtual void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;

    /// <summary>
    /// 衝突終了時の処理  
    /// 接触が解除された際に呼ばれる。
    /// </summary>
    virtual void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

    // ======================
    // ステート・挙動制御
    // ======================

    /// <summary>
    /// ステートを切り替える。KnockBack ステートに情報を渡す場合は
    /// あらかじめ SetPendingDamageInfo() を呼ぶこと。
    /// </summary>
    void ChangeState(const std::string& stateName);

    /// <summary>
    /// HP が 0 になったときに呼ぶ。isAlive_ を false にしてコンポーネントを無効化する。
    /// </summary>
    void OnDeath();

    bool IsAlive() const { return isAlive_; }

    /// <summary>
    /// KnockBack ステートが Enter() で参照するダメージ情報をセットする。
    /// ChangeState("KnockBack") の直前に呼ぶこと。
    /// </summary>
    void SetPendingDamageInfo(const DamageInfo& info) { pendingDamageInfo_ = info; }
    const DamageInfo& GetPendingDamageInfo() const { return pendingDamageInfo_; }

    // ======================
    // Getter / Setter
    // ======================

    /// <summary>
    /// 地面に接地しているかどうかを取得する。
    /// </summary>
    bool GetOnGround() const { return onGround_; }
    void SetOnGround(bool flag) { onGround_ = flag; }

    /// <summary>
    /// プレイヤーのポインタを取得する。
    /// </summary>
    Player* GetPlayer() { return player_; }

    /// <summary>
    /// プレイヤーのポインタを設定する。
    /// </summary>
    void SetPlayer(Player* player) { player_ = player; }

    /// <summary>
    /// 現在の速度ベクトルを取得する。
    /// </summary>
    Vector3 GetVelocity() const { return velocity_; }

    /// <summary>
    /// 現在の速度ベクトルを設定する。
    /// </summary>
    void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }

    /// <summary>
    /// 現在の加速度ベクトルを取得する。
    /// </summary>
    Vector3 GetAcceleration() const { return acceleration_; }

    /// <summary>
    /// 加速度ベクトルを設定する。
    /// </summary>
    void SetAcceleration(const Vector3& acceleration) { acceleration_ = acceleration; }

    /// <summary>
    /// 現在のHPを取得する。
    /// </summary>
    float GetHp() const { return hp_; }

    /// <summary>
    /// HPを設定する。
    /// </summary>
    void SetHp(float hp) { hp_ = hp; }

    bool IsActive() const { return isActive_; }
    void SetActive(bool flag) { isActive_ = flag; }

    void SetIsAttack(bool flag) { isAttack_ = flag; }

    void SetupLockOn(LockOnSystem* lockOnSystem);

protected:
    std::unordered_map<std::string, std::unique_ptr<EnemyStateBase>> states_;
    EnemyStateBase* currentState_ = nullptr;

    LockOnTarget lockOnTarget_;

    Player* player_ = nullptr;

    Vector3 velocity_{};
    Vector3 acceleration_{ 0.0f, 0.0f, 0.0f };

    float hp_ = 3.0f;
    bool onGround_ = false;
    bool isActive_ = true;
    bool isAlive_ = true;
    int deathTimer_ = 0;

    bool isAttack_ = false;

    DamageInfo pendingDamageInfo_;

    std::unique_ptr<HitStop> hitStop_;
};
