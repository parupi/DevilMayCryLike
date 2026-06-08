#pragma once
#include "World3D/Object/Object3d.h"
#include <GameData/Score/StylishScoreManager.h>
#include "Graphics/Rendering/Effect/WeaponTrail.h"

class Player;
class PlayerWeapon : public Object3d
{
public:
	PlayerWeapon(std::string objectName);
	~PlayerWeapon() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update(float deltaTime) override;
	// 描画
	void Draw() override;
	void DrawEffect();

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG
	// 衝突した
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
	// 衝突中
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
	// 離れた
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

	// ======================
	// アクセッサ
	// ======================
	void SetIsAttack(bool flag) { isAttack_ = flag; }
	void SetPlayer(Player* player) { player_ = player; }
	void SetScoreManager(StylishScoreManager* scoreManager) { scoreManager_ = scoreManager; }
private:
	bool isAttack_ = false;

	StylishScoreManager* scoreManager_;
	// プレイヤーの生ポインタ（武器からプレイヤーの状態を参照するために必要）
	Player* player_ = nullptr;
	// 移動と回転の初期位置
	Vector3 defaultPosition_ = { 0.0f, 0.6f, -0.5f };
	Vector3 defaultRotation_ = { 0.0f, 90.0f, 150.0f };

	// 刃先 (tip) と根本 (hilt) のローカルオフセット
	Vector3 tipOffset_  = { 0.0f,  0.5f, 0.0f };
	Vector3 hiltOffset_ = { 0.0f, -0.5f, 0.0f };

	std::unique_ptr<WeaponTrail> trail_;
};

