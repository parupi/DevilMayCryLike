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
	std::unique_ptr<Object3d> object_;
	Vector3 velocity_;

	Input* input = Input::GetInstance();
};

