#pragma once
#include "BaseLight.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

/// <summary>
/// コードから直接パラメータを制御するスポットライト。
/// SpotLight は毎フレーム GlobalVariables から値を読み直すので、コードから向きや位置を変えられない。
/// こちらはキャラクターの攻撃（炎の進行方向を照らす等）のように、所有者が毎フレーム値を入れる用途に使う。
/// </summary>
class DynamicSpotLight : public BaseLight
{
public:
	DynamicSpotLight(const std::string& name);
	~DynamicSpotLight() override = default;

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
	/// <summary>照らす向き（正規化して入れる）</summary>
	void SetDirection(const Vector3& direction);
	void SetIntensity(float intensity) { lightData_.intensity = intensity; }
	/// <summary>光が届く距離[m]</summary>
	void SetDistance(float distance) { lightData_.distance = distance; }
	void SetDecay(float decay) { lightData_.decay = decay; }
	/// <summary>広がりの半角の余弦（0.9 ≒ 25°）</summary>
	void SetCosAngle(float cosAngle) { lightData_.cosAngle = cosAngle; }
};
