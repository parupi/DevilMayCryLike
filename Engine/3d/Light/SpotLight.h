#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
#include <debuger/GlobalVariables.h>
class SpotLight : public BaseLight
{
public:
	SpotLight(const std::string& name);
	~SpotLight() override = default;

	// 初期化処理
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

