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

	// 使用するモデル名を設定する(Initialize()より前に呼ぶこと。未設定時は"Cube")
	void SetModelName(const std::string& modelName) { modelName_ = modelName; }

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

private:
	std::string modelName_ = "Cube";
};

