#pragma once
#include "BaseLight.h"
#include "LightStructs.h"
class DirectionalLight : public BaseLight
{
public:
	DirectionalLight(std::string lightName);

	// 更新処理
	void Update() override;
	// エディターの描画
	void DrawLightEditor();
	// リソースの生成
	void CreateLightResource() override;
	// リソースの更新
	void UpdateLightResource() override;
	// 構造体のサイズを取得
	size_t GetDataSize() const override { return sizeof(DirectionalLightData); }
	// シリアライズ
	void SerializeTo(void* dest) const override;

	//DirectionalLightData& GetLightData() { return lightData_; }
private:

	DirectionalLightData lightData_;
};
