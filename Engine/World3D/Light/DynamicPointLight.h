#pragma once
#include "BaseLight.h"

/// <summary>
/// コードから直接パラメータを制御するポイントライト。
/// GlobalVariables（エディター）に依存せず、キャラクター追従など動的な用途に使う。
/// </summary>
class DynamicPointLight : public BaseLight
{
public:
	DynamicPointLight(const std::string& name);
	~DynamicPointLight() override = default;

	void Initialize() override;
	// 更新処理（パラメータは所有者が毎フレーム設定するため何もしない）
	void Update() override;
#ifdef _DEBUG
	// エディターの描画
	void DrawLightEditor() override;
	// 線描画
	void DrawDebug(PrimitiveLineDrawer* drawer) override;
#endif // DEBUG

	// ======================
	// アクセッサ
	// ======================
	void SetEnabled(bool enabled) { lightData_.enabled = enabled ? 1 : 0; }
	void SetColor(const Vector4& color) { lightData_.color = color; }
	void SetPosition(const Vector3& position) { lightData_.position = position; }
	void SetIntensity(float intensity) { lightData_.intensity = intensity; }
	void SetRadius(float radius) { lightData_.radius = radius; }
	void SetDecay(float decay) { lightData_.decay = decay; }
};
