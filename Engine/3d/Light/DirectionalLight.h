#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
#include <debuger/GlobalVariables.h>
class DirectionalLight : public BaseLight
{
public:
	DirectionalLight(const std::string& name);
	~DirectionalLight() override = default;

	// 初期化処理
	void Initialize() override;
	// 更新処理
	void Update() override;
	// エディターの描画
	void DrawLightEditor();
private:
	GlobalVariables* global_ = nullptr;
};
