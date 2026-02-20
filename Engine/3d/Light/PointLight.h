#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
#include <debuger/GlobalVariables.h>

class PointLight : public BaseLight
{
public:
	PointLight(const std::string& name);
	~PointLight() override = default;

	void Initialize() override;
	// 更新処理
	void Update() override;
#ifdef _DEBUG
	// エディターの描画
	void DrawLightEditor() override;
	// 線描画
	void DrawDebug(PrimitiveLineDrawer* drawer) override;
#endif // DEBUG

private:

	GlobalVariables* global_ = nullptr;
};

