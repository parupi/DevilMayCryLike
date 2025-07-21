#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <3d/Object/Object3dManager.h>
#include <3d/WorldTransform.h>
#include "3d/Object/Renderer/ModelRenderer.h"
#include <input/Input.h>
#include "PlayerAttackEffect.h"
#include "State/PlayerStateBase.h"
#include "State/Attack/PlayerStateAttackBase.h"
#include "PlayerWeapon.h"
#include "math/Vector3.h"
#include "math/function.h"
#include <debuger/GlobalVariables.h>
#include <GameData/Score/StylishScoreManager.h>

class Player : public Object3d
{
public:
	Player(std::string objectNama);
	~Player() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	void DrawEffect();

	// ImGUiによる攻撃のエディター
	void DrawAttackDataEditor(PlayerStateAttackBase* attack);


	// 衝突した
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
	// 衝突中
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
	// 離れた
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;


#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

	// 状態の切り替え
	void ChangeState(const std::string& stateName);

	bool GetOnGround() const { return onGround_; }

	// 移動処理メソッド
	void Move();

	// アクセッサ
	Vector3& GetVelocity() { return velocity_; }
	Vector3& GetAcceleration() { return acceleration_; }
	PlayerAttackEffect* GetPlayerAttackEffect() { return attackEffect_.get(); }
	PlayerWeapon* GetWeapon() { return weapon_.get(); }
	AttackData GetAttackData() const { return attackData_; }
	void SetAttackData(const AttackData& attackData) { attackData_ = attackData; }

private:
	std::unordered_map<std::string, std::unique_ptr<PlayerStateBase>> states_;
	PlayerStateBase* currentState_ = nullptr;

	GlobalVariables* gv = GlobalVariables::GetInstance();
	Input* input = Input::GetInstance();
	// スコア管理用
	std::unique_ptr<StylishScoreManager> scoreManager;

	Vector3 velocity_{};
	Vector3 acceleration_{ 0.0f, 0.0f, 0.0f };

	// 今出している攻撃のパラメータの受け皿
	AttackData attackData_;

	// 攻撃のフラグ
	bool isAttack_ = false;
	float timeAttackCurrent_ = 0.0f;
	float timeAttackMax_ = 0.5f;
	Vector3 startDir_ = { 0.0f, 0.0f, 0.0f };
	Vector3 endDir_ = { 0.0f, 0.0f, 0.0f };

	std::unique_ptr<PlayerAttackEffect> attackEffect_;
	std::unique_ptr<PlayerWeapon> weapon_;

	// 地面と接触しているか
	bool onGround_;
};

