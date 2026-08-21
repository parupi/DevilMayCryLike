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
#include "GameObject/Character/MovementBounds.h"
#include "GameObject/Effect/HitVignetteEffect.h"
#include "GameObject/Effect/HitPostEffect.h"
#include "GameObject/Effect/CharacterLight.h"
#include "GameObject/Effect/HitFlashComponent.h"
#include "GameObject/Effect/DeathScreenEffect.h"
#include "GameObject/Effect/DissolveOutEffect.h"
#include "Combat/PlayerCombat.h"
#include "GameObject/LockOn/LockOnSystem.h"
#include "Tutorial/Service/TutorialService.h"

class PlayerInput;
class AnimationPlayer;

struct PlayerCommand;

/// <summary>
/// プレイヤーキャラクターを制御するクラス  
/// 
/// モデル描画、状態遷移、移動、攻撃、ロックオン、スコア加算、  
/// エフェクトやヒットストップなど、プレイヤーの全挙動を管理する。
/// </summary>
class Player : public Object3d {
public:
	/// 見た目のモデル。Resource/models/Player/Alien/Alien.obj を指す
	/// （ヘルメット付きにするなら "Player/Alien_Helmet" に変えるだけでよい）
	static constexpr const char* kModelName = "Player/Alien";
	/// 素の高さ約2.9m を約1.1m にするスケール。大きさを変えるならここ
	static constexpr float kModelScale = 0.38f;
	/// レンダラーの登録名。モデル名とは別物で、Player 内から GetRenderer() で引くのに使う
	static constexpr const char* kRendererName = "PlayerModel";
	/// モデルの足元(y=0)をコライダーの底に合わせるための縦オフセット
	static constexpr float kModelOffsetY = -0.5f;

	// ── アニメーションクリップ名（Alien.gltf が持つ15種のうち使うもの）──
	// 差し替えは Player::UpdateAnimation() の対応表と合わせて見ること
	static constexpr const char* kClipIdle      = "Alien_Idle";
	static constexpr const char* kClipMove      = "Alien_Run";
	static constexpr const char* kClipJump      = "Alien_Jump";
	static constexpr const char* kClipAttack    = "Alien_SwordSlash";
	static constexpr const char* kClipKnockBack = "Alien_Roll";
	static constexpr const char* kClipDeath     = "Alien_Death";
	static constexpr const char* kClipClear     = "Alien_Clapping";

	/// Alien_SwordSlash(1.04秒)で振り切る瞬間の位置。
	/// gltf のキーフレームで Palm.R / Torso の角速度ピークが 0.458秒＝44%だった。
	/// クリップを差し替えたら測り直すこと
	static constexpr float kAttackClipImpactRatio = 0.44f;
	/// 攻撃の再生速度の上下限。0.15秒しかない空中攻撃で倍率が跳ね上がって残像になるのを防ぐ
	static constexpr float kAttackSpeedMin = 0.75f;
	static constexpr float kAttackSpeedMax = 3.0f;

	/// とどめの一撃で入れるヒットストップ。普段の攻撃（0.01〜0.03秒）よりはっきり長く止める
	static constexpr float kDeathHitStopTime = 0.18f;
	static constexpr float kDeathHitStopIntensity = 0.15f;

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
	StylishScoreManager* GetScoreManager() { return scoreManager.get(); }
	PlayerStateMachine* GetStateMachine() { return stateMachine_.get(); }

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
	/// <summary>
	/// 攻撃ヒット時のポストエフェクト（放射状ブラー・色収差・フラッシュ）を取得する。
	/// </summary>
	HitPostEffect* GetHitPostEffect() const { return hitPostEffect_.get(); }
	bool IsAttack() const { return combat_->IsAttacking(); }

	/// <summary>
	/// プレイヤーに追従するポイントライトを取得する。
	/// 攻撃ヒット時に Flash() を呼ぶとひときわ強く光る。
	/// </summary>
	CharacterLight* GetCharacterLight() const { return characterLight_.get(); }

	// 被ダメージ処理
	void TakeDamage(const DamageInfo& info);
	const DamageInfo& GetPendingDamageInfo() const { return pendingDamageInfo_; }

	int32_t GetHp() const { return hp_; }
	int32_t GetMaxHp() const { return maxHp_; }
	/// <summary>HPを直接設定する（トレーニングのリセット用）。0以下にしても死亡処理は走らない</summary>
	void SetHp(int32_t hp) { hp_ = hp; }

	/// <summary>
	/// true の間、被弾しない（TakeDamage が何もしない）。
	/// 敵の攻撃モーションを何度も見たいときに使う、トレーニング用のスイッチ
	/// </summary>
	void SetInvincible(bool invincible) { invincible_ = invincible; }
	bool IsInvincible() const { return invincible_; }

	/// <summary>
	/// 死亡演出が終わったことを知らせる。PlayerStateDeath から呼ぶ。
	///
	/// シーンを切り替えるのはプレイヤーの仕事ではないので、
	/// ここで印を付けておいて GameScene 側に拾ってもらう
	/// </summary>
	void NotifyDeathFinished() { isDeathFinished_ = true; }
	bool IsDeathFinished() const { return isDeathFinished_; }

	/// <summary>
	/// 死亡演出中か（Death ステートにいるか）。演出が終わってもステートは Death のままなので、
	/// ゲームオーバーの選択中も true を返す。
	/// 世界の時間を止める・HUDを引っ込める、といった判断に使う
	/// </summary>
	bool IsDying() const;

	/// <summary>体のアニメーション再生窓口。静的モデルを使っている間は nullptr が返る</summary>
	AnimationPlayer* GetAnimationPlayer();

	/// <summary>死亡演出の画面効果（グレースケール＋暗転ビネット）</summary>
	DeathScreenEffect* GetDeathScreen() const { return deathScreen_.get(); }
	/// <summary>死亡演出の消滅（ディゾルブ＋黒いもや）</summary>
	DissolveOutEffect* GetDeathDissolve() const { return deathDissolve_.get(); }

	/// <summary>HUD（ハート）の不透明度。死亡演出でフェードアウトさせるのに使う</summary>
	void SetHudAlpha(float alpha) { hudAlpha_ = alpha; }

	void SetInput(PlayerInput* input) { input_ = input; }
	void SetLockOn(LockOnSystem* lockOn) { lockOn_ = lockOn; }
	void SetTutorialService(TutorialService* tutorialService) { tutorialService_ = tutorialService; }
	TutorialService* GetTutorialService() const { return tutorialService_; }

	// 移動可能範囲(水平方向)を設定する。強制戦闘イベントなどでプレイヤーをエリア内に閉じ込めるのに使う。
	void SetMovementBounds(const MovementBounds& bounds) {
		movementBounds_ = bounds; hasMovementBounds_ = true;
	}
	// 移動可能範囲の制限を解除する。
	void ClearMovementBounds() { hasMovementBounds_ = false; }
private:
	// 地形コライダーとのめり込みを解消する（OnCollisionEnter/Stay共通処理）
	void ResolveGroundCollision(BaseCollider* other);
	// 敵とのめり込みを水平方向だけで解消する（OnCollisionEnter/Stay共通処理）
	void ResolveCharacterCollision(BaseCollider* other);

	// ステートと戦闘状態から再生するクリップを決めて流す。毎フレーム呼ぶ
	void UpdateAnimation();

	// 直前のフレームに再生していた攻撃名。コンボで技が変わったら振りを出し直すために覚えておく
	std::string lastAttackName_;

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
	// 常時無敵（トレーニング用。通常のプレイでは false のまま）
	bool invincible_ = false;
	// 死亡演出を最後まで再生し終えたか
	bool isDeathFinished_ = false;
	// 被ダメージ情報（ノックバックステートで参照）
	DamageInfo pendingDamageInfo_;
	// 被弾時のビネットエフェクト
	std::unique_ptr<HitVignetteEffect> hitVignette_;
	std::unique_ptr<HitPostEffect> hitPostEffect_;
	// プレイヤーに追従するポイントライト（攻撃ヒット時にフラッシュ）
	std::unique_ptr<CharacterLight> characterLight_;
	// 被弾時に体と武器を一瞬白く光らせるコンポーネント（EmissiveTintを使う）
	std::unique_ptr<HitFlashComponent> hitFlash_;
	// 死亡演出で画面から色を抜き、視界を閉じていくエフェクト
	std::unique_ptr<DeathScreenEffect> deathScreen_;
	// 死亡演出の最後に体と武器を溶かして消すエフェクト
	std::unique_ptr<DissolveOutEffect> deathDissolve_;
	// HUD（ハート）の不透明度。死亡演出で 1 → 0 にする
	float hudAlpha_ = 1.0f;

	// 移動可能範囲(水平方向)。強制戦闘イベント発動中などに有効化される。
	bool hasMovementBounds_ = false;
	MovementBounds movementBounds_{};
};
