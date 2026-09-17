#pragma once
#include "World3D/Object/Object3d.h"
class Ground : public Object3d
{
public:
	Ground(std::string objectName);
	~Ground() override = default;
	// 初期
	void Initialize() override;
	// 更新
	void Update(float deltaTime) override;
	// 描画
	void Draw() override;

	// 使用するモデル名は Object3d::SetModelName()。未設定なら Initialize() で "Cube" になる
};

