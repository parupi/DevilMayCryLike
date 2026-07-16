#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <World3D/Object/Object3dManager.h>
#include <World3D/WorldTransform.h>
#include "World3D/Object/Renderer/ModelRenderer.h"
#include "State/PlayerStateBase.h"
#include "PlayerWeapon.h"
#include "Math/Vector3.h"
#include "Math/MathUtils.h"
#include <GameData/Score/StylishScoreManager.h>
#include <Graphics/Rendering/Sprite/Sprite.h>
#include <GameObject/Effect/HitStop.h>
#include "StateMachine/PlayerStateMachine.h"
#include "GameObject/Character/CharacterStructs.h"
#include "GameObject/Effect/HitVignetteEffect.h"
#include "Combat/PlayerCombat.h"
#include "GameObject/LockOn/LockOnSystem.h"
#include "Tutorial/Service/TutorialService.h"

class PlayerInput;

struct PlayerCommand;

/// <summary>
/// プレイヤーキャラクターを制御するクラス  
/// 
/// モデル描画、状態遷移、移動、攻撃、ロックオン、スコア加算、  
/// エフェクトやヒットストップなど、プレイヤーの全挙動を管理する。
/// </summary>
class Player : public Object3d {
public:
	Player(std::string objectName);
	~Player() override = default;

	/// <summary>
	/// プレイヤーの初期化処理  
	/// モデル、ステート、エフェクト、武器、スコアなどを初期化する。
	/// </summary>
	void Initialize() override;

	/// <summary>
	/// プレイヤーの更新処理  
	/// 入力、移動、状態遷移、攻撃、ロックオンなどの制御を行う。
	/// </summary>
	void Update(float deltaTime) override;

	/// <summary>
	/// プレイヤーの描画処理  
	/// モデルや武器などの3D描画を実行する。
	/// </summary>
	void Draw() override;

	/// <summary>
	/// エフェクト描画処理  
	/// 攻撃時エフェクトやヒット演出などを描画する。
	/// </summary>
	void DrawEffect();
	// UI描画処理
	void DrawUI();

	/// <summary>
	/// 衝突開始時の処理  
	/// 他オブジェクトとの初回接触時に呼ばれる。
	/// </summary>
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;

	/// <summary>
	/// 衝突中の処理  
	/// 毎フレーム呼ばれ、接触継続中の処理を行う。
	/// </summary>
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;

	/// <summary>
	/// 衝突終了時の処理  
	/// 接触が解除されたときに呼ばれる。
	/// </summary>
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

#ifdef _DEBUG
	/// <summary>
	/// デバッグ用GUI描画処理  
	/// ImGuiを用いて内部情報（速度・ステートなど）を可視化する。
	/// </summary>
	void DebugGui() override;
#endif // _DEBUG

	/// <summary>
	/// プレイヤーの状態を切り替える。  
	/// 攻撃、移動、待機などのステート遷移を管理する。
	/// </summary>
	/// <param name="stateName">遷移先のステート名</param>
	void ChangeState(const std::string& stateName);

	void ExecuteCommand(const PlayerCommand& command);

	/// <summary>
	/// 地面に接地しているかどうかを取得する。
	/// </summary>
	bool GetOnGround() const { return onGround_; }

	// プレイヤーの移動方向を取得する。
	Vector3 GetMoveDirection() const;
	// プレイヤーの移動処理  
	void Move(Vector3 moveDir, float deltaTime);
	// プレイヤーの向き更新処理
	void Rotate(Vector3 moveDir, float deltaTime);
	/// ロックオン処理  
	void LockOn();

	PlayerCombat* GetCombat() { return combat_.get(); }
	PlayerInput* GetInput() { return input_; }

	// ======================
	// アクセッサ
	// ======================

	Vector3& GetVelocity() { return velocity_; } ///< 現在の速度ベクトルを取得
	Vector3& GetAcceleration() { return acceleration_; } ///< 現在の加速度ベクトルを取得
	PlayerWeapon* GetWeapon() { return weapon_.get(); } ///< プレイヤーの武器クラス取得
	AttackData GetAttackData() const { return attackData_; } ///< 現在の攻撃データを取得
	void SetAttackData(const AttackData& attackData) { attackData_ = attackData; } ///< 攻撃データを設定

	/// <summary>
	/// 現在ロックオンしているかどうかを取得する。
	/// </summary>
	bool IsLockOn() const { return lockOn_->IsLockOn(); }

	HitStop* GetHitStop() const { return hitStop_.get(); }
	bool IsAttack() const { return combat_->IsAttacking(); }

	// 被ダメージ処理
	void TakeDamage(const DamageInfo& info);
	const DamageInfo& GetPendingDamageInfo() const { return pendingDamageInfo_; }

	int32_t GetHp() const { return hp_; }

	void SetInput(PlayerInput* input) { input_ = input; }
	void SetLockOn(LockOnSystem* lockOn) { lockOn_ = lockOn; }
	void SetTutorialService(TutorialService* tutorialService) { tutorialService_ = tutorialService; }
	TutorialService* GetTutorialService() const { return tutorialService_; }

	// 移動可能範囲(XZ)を設定する。強制戦闘イベントなどでプレイヤーをエリア内に閉じ込めるのに使う。
	void SetMovementBounds(const Vector3& min, const Vector3& max) {
		movementBoundsMin_ = min; movementBoundsMax_ = max; hasMovementBounds_ = true;
	}
	// 移動可能範囲の制限を解除する。
	void ClearMovementBounds() { hasMovementBounds_ = false; }
private:
	// Ground/Enemyコライダーとのめり込みを解消する（OnCollisionEnter/Stay共通処理）
	void ResolveGroundCollision(BaseCollider* other);

	std::unique_ptr<PlayerStateMachine> stateMachine_ = nullptr;

	std::unique_ptr<PlayerCombat> combat_ = nullptr;

	PlayerInput* input_ = nullptr;

	LockOnSystem* lockOn_ = nullptr;

	// チュートリアルへゲームプレイのイベントを伝えるためのサービス
	TutorialService* tutorialService_ = nullptr;

	GlobalVariables* gv = &GlobalVariables::GetInstance(); ///< グローバル変数管理

	std::unique_ptr<StylishScoreManager> scoreManager; ///< スタイリッシュスコア管理クラス

	Vector3 velocity_{}; ///< プレイヤーの速度
	Vector3 acceleration_{0.0f, 0.0f, 0.0f}; ///< プレイヤーの加速度

	AttackData attackData_; ///< 現在実行中の攻撃データ

	std::unique_ptr<HitStop> hitStop_;

	std::unique_ptr<PlayerWeapon> weapon_; ///< 武器クラス
	// 接地判定フラグ
	bool onGround_ = false;
	// HPのハートのスプライト
	std::vector<Sprite*> hearts_;
	// 移動速度
	const float moveSpeed_ = 10.0f;
	// 回転速度
	const float rotateSpeed_ = 5.0f;
	// 最大HP
	int32_t maxHp_ = 5;
	// HP
	int32_t hp_ = 5;
	// 無敵時間（被弾直後の連続ヒット防止）
	float invincibleTimer_ = 0.0f;
	// 被ダメージ情報（ノックバックステートで参照）
	DamageInfo pendingDamageInfo_;
	// 被弾時のビネットエフェクト
	std::unique_ptr<HitVignetteEffect> hitVignette_;

	// 移動可能範囲(XZ)。強制戦闘イベント発動中などに有効化される。
	bool hasMovementBounds_ = false;
	Vector3 movementBoundsMin_{};
	Vector3 movementBoundsMax_{};
};
