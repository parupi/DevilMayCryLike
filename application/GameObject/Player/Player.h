#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <Object3d.h>
#include <WorldTransform.h>
#include <Input.h>
class Player
{
public:
	// 初期
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw();


private:
	std::unique_ptr<Object3d> objectBody_;
	std::unique_ptr<Object3d> objectHead_;
	std::unique_ptr<Object3d> objectLeftArm_;
	std::unique_ptr<Object3d> objectRightArm_;
	Vector3 velocity_;

	Input* input = Input::GetInstance();
	// 攻撃のフラグ
	bool isAttack_ = false;
	float timeAttackCurrent_ = 0.0f;
	float timeAttackMax_ = 0.5f;
	Vector3 startDir_ = { 0.0f, 0.0f, 0.0f };
	Vector3 endDir_ = { 0.0f, 0.0f, 0.0f };
	
};

