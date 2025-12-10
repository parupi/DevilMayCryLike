#pragma once
#include "3d/Object/Object3d.h"
#include "base/Particle/ParticleEmitter.h"
#include "BaseState/EnemyStateBase.h"
#include "GameObject/Effect/HitStop.h"
#include "GameObject/Character/CharacterStructs.h"

class Player;

/// <summary>
/// 敵キャラクターを制御するクラス  
/// 
/// 状態遷移、衝突処理、被ダメージ・死亡演出、ヒットストップ、
/// パーティクルエフェクトなどを統合的に管理する。
/// </summary>
class Enemy : public Object3d
{
public:
    Enemy(std::string objectName);
    virtual ~Enemy() override = default;

    /// <summary>
    /// 敵の初期化処理  
    /// 各種ステート・エフェクト・物理パラメータを設定する。
    /// </summary>
    virtual void Initialize() override;

    /// <summary>
    /// 敵の更新処理  
    /// 現在の状態（ステート）に応じた行動・移動・攻撃を行う。
    /// </summary>
    virtual void Update() override;

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
    /// 敵の状態（ステート）を切り替える。  
    /// 例：待機 → 攻撃、攻撃 → 被弾、など。
    /// </summary>
    /// <param name="stateName">遷移先ステート名</param>
    void ChangeState(const std::string& stateName, const DamageInfo* info = nullptr);

    /// <summary>
    /// 敵の死亡処理  
    /// HPが0になった際のエフェクトや消滅処理を行う。
    /// </summary>
    void OnDeath();

    /// <summary>
    /// 敵が生存しているかを返す。
    /// </summary>
    bool IsAlive() const { return isAlive_; }

    // 食らった攻撃のパラメータを保存
    void HitDamage();

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

    /// <summary>
    /// 敵がアクティブ状態かどうかを取得する。
    /// </summary>
    bool IsActive() const { return isActive_; }

    /// <summary>
    /// 敵のアクティブ状態を設定する。
    /// </summary>
    void SetActive(bool flag) { isActive_ = flag; }

    // 煙を出す
    void EmitSmoke() { smokeEmitter_->Emit(); }

protected:
    std::unordered_map<std::string, std::unique_ptr<EnemyStateBase>> states_; ///< ステート名と対応するステートオブジェクト
    EnemyStateBase* currentState_ = nullptr; ///< 現在のステート

    std::unique_ptr<ParticleEmitter> slashEmitter_; ///< 被弾・斬撃エフェクト用パーティクル
    std::unique_ptr<ParticleEmitter> smokeEmitter_; ///< 被弾・斬撃エフェクト用パーティクル

    Player* player_ = nullptr; ///< プレイヤー参照ポインタ

    Vector3 velocity_{}; ///< 現在の速度
    Vector3 acceleration_{ 0.0f, 0.0f, 0.0f }; ///< 現在の加速度

    float hp_ = 3.0f; ///< 敵のHP
    bool onGround_ = false; ///< 地面との接地判定
    bool isActive_ = true; ///< アクティブ状態（処理対象か）
    bool isAlive_ = true; ///< 生存状態フラグ
    int deathTimer_ = 0; ///< 死亡後の待機タイマー

    DamageInfo damageInfo_;

    std::unique_ptr<HitStop> hitStop_; ///< ヒットストップ管理クラス
};
