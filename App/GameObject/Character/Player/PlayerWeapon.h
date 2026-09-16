#pragma once
#include "World3D/Object/Object3d.h"
#include <GameData/Score/StylishScoreManager.h>

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
	/// <summary>
	/// 次の当たり判定を1回だけ切る（多段ヒット用）。
	/// 触れたままの敵に、もう一度 OnCollisionEnter を起こすために使う
	/// </summary>
	void RequestRehit() { rehitRequested_ = true; }

	/// <summary>刃先のワールド座標（今のワールド行列から計算する）。剣の軌跡とヒット位置に使う</summary>
	Vector3 GetBladeTipWorld();
	/// <summary>刃の根本（鍔のすぐ上）のワールド座標</summary>
	Vector3 GetBladeBaseWorld();
private:
	bool isAttack_ = false;
	// RequestRehit で立てる。次の Update で判定を1回切ったら下ろす
	bool rehitRequested_ = false;
	// 当たり判定（OBB）の基準の大きさ。攻撃ごとの倍率（Hitbox Scale）はこれに掛ける
	Vector3 baseHalfExtents_ = { 0.5f, 1.0f, 0.5f };

	StylishScoreManager* scoreManager_;
	// プレイヤーの生ポインタ（武器からプレイヤーの状態を参照するために必要）
	Player* player_ = nullptr;
	// 移動と回転の初期位置
	Vector3 defaultPosition_ = { 0.0f, 0.6f, -0.5f };
	Vector3 defaultRotation_ = { 0.0f, 90.0f, 150.0f };

	// 刃先と刃の根本のローカル位置。Sword.obj は Y 方向に -1.31〜1.43 で、
	// -1.4〜-0.8 が柄、-0.8〜-0.7 が鍔、その上が刃（先端 1.43）
	Vector3 bladeTipOffset_ = { 0.0f, 1.35f, 0.0f };
	Vector3 bladeBaseOffset_ = { 0.0f, -0.65f, 0.0f };
};

