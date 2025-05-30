#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <Object3d.h>
#include <WorldTransform.h>
#include "3d/Object/Renderer/ModelRenderer.h"
#include <Input.h>
#include "PlayerAttackEffect.h"
#include "State/PlayerStateBase.h"

class Player : public Object3d
{
public:
	Player();
	~Player() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update() override;
	// 描画
	void Draw() override;
	void DrawEffect();

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

	// 状態の切り替え
	void ChangeState(const std::string& stateName);


	// アクセッサ
	Vector3& GetVelocity() { return velocity_; }
	PlayerAttackEffect* GetPlayerAttackEffect() { return attackEfect_.get(); }

private:
	std::unordered_map<std::string, std::unique_ptr<PlayerStateBase>> states_;
	PlayerStateBase* currentState_ = nullptr;

	Vector3 velocity_{};

	Input* input = Input::GetInstance();
	// 攻撃のフラグ
	bool isAttack_ = false;
	float timeAttackCurrent_ = 0.0f;
	float timeAttackMax_ = 0.5f;
	Vector3 startDir_ = { 0.0f, 0.0f, 0.0f };
	Vector3 endDir_ = { 0.0f, 0.0f, 0.0f };
	
	std::unique_ptr<PlayerAttackEffect> attackEfect_;


};

