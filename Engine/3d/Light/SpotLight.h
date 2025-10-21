#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class SpotLight : public BaseLight
{
public:
	SpotLight(std::string lightName);
	~SpotLight() override = default;

	// 更新処理
	void Update() override;
	// エディターの描画
	void DrawLightEditor();
	// リソースの生成
	void CreateLightResource() override;
	// リソースの更新
	void UpdateLightResource() override;
	// 構造体のサイズを取得
	size_t GetDataSize() const override { return sizeof(PointLightData); }
	// シリアライズ
	void SerializeTo(void* dest) const override;

	SpotLightData& GetLightData() { return lightData_; }

private:
	SpotLightData lightData_;
};

