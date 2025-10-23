#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <3d/Object/Object3dManager.h>
#include <3d/WorldTransform.h>
#include "3d/Object/Renderer/ModelRenderer.h"
#include <input/Input.h>
#include "State/PlayerStateBase.h"
#include "State/Attack/PlayerStateAttackBase.h"
#include "PlayerWeapon.h"
#include "math/Vector3.h"
#include "math/function.h"
#include <debuger/GlobalVariables.h>
#include <GameData/Score/StylishScoreManager.h>
#include <GameObject/Enemy/Enemy.h>
#include <2d/Sprite.h>
#include <GameObject/Effect/HitStop.h>

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
	void Update() override;

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
	/// 攻撃データの編集UIを表示する（ImGui用）  
	/// 攻撃ステートごとのパラメータを可視化・編集する。
	/// </summary>
	/// <param name="attack">編集対象の攻撃ステート</param>
	void DrawAttackDataEditor(PlayerStateAttackBase* attack);

	/// <summary>
	/// 攻撃データ編集用の全体UIを表示する（ImGui用）
	/// </summary>
	void DrawAttackDataEditorUI();

	/// <summary>
	/// インデックスから攻撃ステート名を取得する。
	/// </summary>
	/// <param name="index">攻撃ステートのインデックス</param>
	/// <returns>攻撃ステート名</returns>
	std::string GetAttackStateNameByIndex(int32_t index) const;

	/// <summary>
	/// 登録されている攻撃ステートの総数を取得する。
	/// </summary>
	/// <returns>攻撃ステート数</returns>
	int32_t GetAttackStateCount() const;

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

	// ======================
	// アクセッサ
	// ======================

	Vector3& GetVelocity() { return velocity_; } ///< 現在の速度ベクトルを取得
	Vector3& GetAcceleration() { return acceleration_; } ///< 現在の加速度ベクトルを取得
	//PlayerAttackEffect* GetPlayerAttackEffect() { return attackEffect_.get(); } ///< 攻撃エフェクト管理クラス取得
	PlayerWeapon* GetWeapon() { return weapon_.get(); } ///< プレイヤーの武器クラス取得
	AttackData GetAttackData() const { return attackData_; } ///< 現在の攻撃データを取得
	void SetAttackData(const AttackData& attackData) { attackData_ = attackData; } ///< 攻撃データを設定

	/// <summary>
	/// ロックオン中の敵のワールド座標を取得する。
	/// </summary>
	const Vector3& GetLockOnPos() { return lockOnEnemy_->GetWorldTransform()->GetTranslation(); }

	/// <summary>
	/// 現在ロックオンしているかどうかを取得する。
	/// </summary>
	bool IsLockOn() const { return isLockOn_; }

	/// <summary>
	/// ヒットストップクラスを取得する。
	/// </summary>
	HitStop* GetHitStop() const { return hitStop_.get(); }

private:
	std::unordered_map<std::string, std::unique_ptr<PlayerStateBase>> states_; ///< ステート名とステートインスタンスのマップ
	PlayerStateBase* currentState_ = nullptr; ///< 現在のステート

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

	//std::unique_ptr<PlayerAttackEffect> attackEffect_; ///< 攻撃エフェクトクラス
	std::unique_ptr<PlayerWeapon> weapon_; ///< 武器クラス

	bool onGround_ = false; ///< 接地判定フラグ

	std::unique_ptr<Sprite> titleWord_; ///< テスト用スプライト（デバッグ表示など）
};
