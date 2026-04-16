#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <3d/Object/Object3dManager.h>
#include <3d/WorldTransform.h>
#include "3d/Object/Renderer/ModelRenderer.h"
#include "State/PlayerStateBase.h"
#include "PlayerWeapon.h"
#include "math/Vector3.h"
#include "math/function.h"
#include <debuger/GlobalVariables.h>
#include <GameData/Score/StylishScoreManager.h>
#include <2d/Sprite.h>
#include <GameObject/Effect/HitStop.h>
//#include "State/Attack/AttackBranchUI.h"
#include "StateMachine/PlayerStateMachine.h"
//#include "Movement/PlayerMovement.h"
//#include "Collision/PlayerCollider.h"
//#include "Collision/PlayerCollisionResolver.h"
#include "Combat/PlayerCombat.h"
//#include "GameObject/Effect/HitStopComponent.h"
#include "GameObject/LockOn/LockOnSystem.h"

class PlayerInput;

struct PlayerCommand;

/// <summary>
/// プレイヤーキャラクターを制御するクラス  
/// 
/// モデル描画、状態遷移、移動、攻撃、ロックオン、スコア加算、  
/// エフェクトやヒットストップなど、プレイヤーの全挙動を管理する。
/// </summary>
class Player : public Object3d
{
public:
	Player(std::string objectNama);
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

	/// <summary>
	/// プレイヤーの移動処理  
	/// 入力ベクトルと物理挙動（加速度・速度）をもとに位置を更新する。
	/// </summary>
	void Move(float deltaTime);

	/// <summary>
	/// ロックオン処理  
	/// ターゲットを固定する。
	/// </summary>
	void LockOn();

	const Vector3& GetMoveVector() { return moveVector_; }

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

	// 攻撃派生UI
	//AttackBranchUI* GetAttackBranchUI() { return attackBranchUI_.get(); }

	HitStop* GetHitStop() const { return hitStop_.get(); }
	bool IsAttack() const { return combat_->IsAttacking(); }

	void SetInput(PlayerInput* input) { input_ = input; }
	void SetLockOn(LockOnSystem* lockOn) { lockOn_ = lockOn; }
private:
	std::unique_ptr<PlayerStateMachine> stateMachine_ = nullptr;

	std::unique_ptr<PlayerCombat> combat_ = nullptr;

	PlayerInput* input_ = nullptr;

	LockOnSystem* lockOn_ = nullptr;

	GlobalVariables* gv = GlobalVariables::GetInstance(); ///< グローバル変数管理

	std::unique_ptr<StylishScoreManager> scoreManager; ///< スタイリッシュスコア管理クラス

	Vector3 velocity_{}; ///< プレイヤーの速度
	Vector3 acceleration_{ 0.0f, 0.0f, 0.0f }; ///< プレイヤーの加速度

	AttackData attackData_; ///< 現在実行中の攻撃データ

	std::unique_ptr<HitStop> hitStop_;

	std::unique_ptr<PlayerWeapon> weapon_; ///< 武器クラス
	// 接地判定フラグ
	bool onGround_ = false;
	// ロックオン時のレティクル
	Sprite* reticle_;
	// 1フレームの移動距離を保持
	Vector3 moveVector_;
	// 調整用
	float rotateSpeed_ = 5.0f;
};
