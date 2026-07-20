#pragma once
#include "World3D/Object/Object3d.h"

class DynamicPointLight;

/// <summary>
/// レベルエディタで配置するポイントライト。
/// 描画物は持たず、自分のトランスフォーム位置にポイントライトを生成する。
/// パラメータ（色・強さ・半径・減衰）は SceneBuilder が Initialize() より前に設定する。
/// </summary>
class StagePointLight : public Object3d
{
public:
	StagePointLight(std::string objectName);
	~StagePointLight() override;

	void Initialize() override;
	void Update(float deltaTime) override;

	/// <summary>ライトのパラメータを設定する（Initialize() より前に呼ぶこと）</summary>
	/// <param name="offset">オブジェクト原点からのオフセット（エンジン座標系）</param>
	void SetLight(const Vector3& color, const Vector3& offset, float intensity, float radius, float decay) {
		color_ = color;
		offset_ = offset;
		intensity_ = intensity;
		radius_ = radius;
		decay_ = decay;
	}

private:
	DynamicPointLight* light_ = nullptr; // 所有権は LightManager

	Vector3 color_ = { 1.0f, 1.0f, 1.0f };
	Vector3 offset_ = { 0.0f, 0.0f, 0.0f };
	float intensity_ = 1.5f;
	float radius_ = 10.0f;
	float decay_ = 1.0f;
};
