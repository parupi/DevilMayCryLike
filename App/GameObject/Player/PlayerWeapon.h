#pragma once
#include "3d/Object/Object3d.h"
class PlayerWeapon : public Object3d
{
public:
	PlayerWeapon(std::string objectName);
	~PlayerWeapon() override = default;
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
	// 衝突した
	void OnCollisionEnter([[maybe_unused]] BaseCollider* other) override;
	// 衝突中
	void OnCollisionStay([[maybe_unused]] BaseCollider* other) override;
	// 離れた
	void OnCollisionExit([[maybe_unused]] BaseCollider* other) override;

	void SetIsAttack(bool flag) { isAttack_ = flag; }
private:
	bool isAttack_ = false;

};

