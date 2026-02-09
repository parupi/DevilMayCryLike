#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <3d/Object/Object3dManager.h>
#include <3d/WorldTransform.h>
#include "3d/Object/Renderer/ModelRenderer.h"
#include <input/Input.h>
#include "State/PlayerStateBase.h"
#include "PlayerWeapon.h"
#include "math/Vector3.h"
#include "math/function.h"
#include <debuger/GlobalVariables.h>
#include <GameData/Score/StylishScoreManager.h>
#include <GameObject/Character/Enemy/Enemy.h>
#include <2d/Sprite.h>
#include <GameObject/Effect/HitStop.h>
#include "State/Attack/AttackBranchUI.h"
#include "StateMachine/PlayerStateMachine.h"
#include "Movement/PlayerMovement.h"
#include "Collision/PlayerCollider.h"
#include "Collision/PlayerCollisionResolver.h"
#include "Combat/PlayerCombat.h"

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
	void Move();

	/// <summary>
	/// ロックオン処理  
	/// 敵との距離・方向を判定し、ターゲットを固定する。
	/// </summary>
	void LockOn();

	void Clear();

	//void SetIntent(const MoveIntent& intent);

	void RequestAttack(AttackType id);

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
	/// ロックオン中の敵のワールド座標を取得する。
	/// </summary>
	const Vector3& GetLockOnPos();

	/// <summary>
	/// 現在ロックオンしているかどうかを取得する。
	/// </summary>
	bool IsLockOn() const { return isLockOn_; }

	bool IsClear() const { return isClear_; }

	/// <summary>
	/// ヒットストップクラスを取得する。
	/// </summary>
	HitStop* GetHitStop() const { return hitStop_.get(); }

	// 攻撃派生UI
	AttackBranchUI* GetAttackBranchUI() { return attackBranchUI_.get(); }


	AttackInputState GetAttackInputState() const;

	void SetInput(PlayerInput* input) { input_ = input; }
private:
	std::unique_ptr<PlayerStateMachine> stateMachine_ = nullptr;

	std::unique_ptr<PlayerCombat> combat_ = nullptr;

	std::unique_ptr<PlayerMovement> movement_ = nullptr;

	std::unique_ptr<PlayerCollider> collider_ = nullptr;
	
	std::unique_ptr<PlayerCollisionResolver> collisionResolver_ = nullptr;

	PlayerInput* input_ = nullptr;

	GlobalVariables* gv = GlobalVariables::GetInstance(); ///< グローバル変数管理
	Input* input = Input::GetInstance(); ///< 入力管理クラス
	std::unique_ptr<StylishScoreManager> scoreManager; ///< スタイリッシュスコア管理クラス

	Vector3 velocity_{}; ///< プレイヤーの速度
	Vector3 acceleration_{ 0.0f, 0.0f, 0.0f }; ///< プレイヤーの加速度

	AttackData attackData_; ///< 現在実行中の攻撃データ
	std::unique_ptr<HitStop> hitStop_; ///< ヒットストップ管理クラス

	std::vector<Enemy*> enemies_; ///< 周囲の敵リスト
	Enemy* lockOnEnemy_ = nullptr; ///< 現在ロックオンしている敵
	bool isLockOn_ = false; ///< ロックオン状態フラグ

	std::unique_ptr<PlayerWeapon> weapon_; ///< 武器クラス

	bool onGround_ = false; ///< 接地判定フラグ

	std::unique_ptr<Sprite> titleWord_; ///< テスト用スプライト（デバッグ表示など）

	bool isClear_ = false;

	std::unique_ptr<AttackBranchUI> attackBranchUI_;
};
