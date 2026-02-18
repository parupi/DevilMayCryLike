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
	// エディターの描画
	void DrawLightEditor();
	//// リソースの生成
	//void CreateLightResource() override;
	//// リソースの更新
	//void UpdateLightResource() override;
	//// 構造体のサイズを取得
	//size_t GetDataSize() const override { return sizeof(PointLightData); }
	//// シリアライズ
	//void SerializeTo(void* dest) const override;

	//// アクセッサ
	//void SetPosition(const Vector3& p) { lightData_.position = p; MarkDirty(); }
	//void SetColor(const Vector4& c) { lightData_.color = c; MarkDirty(); }
	//void SetRadius(float r) { lightData_.radius = r; MarkDirty(); }
	//void SetIntensity(float i) { lightData_.intensity = i; MarkDirty(); }
private:
	//PointLightData lightData_{};

	GlobalVariables* global_ = nullptr;
};

