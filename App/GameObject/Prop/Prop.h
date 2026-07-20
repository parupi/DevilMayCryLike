#pragma once
#include "World3D/Object/Object3d.h"

class DynamicPointLight;

/// <summary>
/// レベルエディタで配置する小物（装飾オブジェクト）。
/// file_name で指定したモデルを表示する。ランタンのような発光する小物のために
/// オプションでポイントライトを持てる（レベルエディタの Prop パネルで設定）。
/// コライダーはレベルエディタで付けた場合のみ生成され、Ground カテゴリ（押し出し対象）になる。
/// </summary>
class Prop : public Object3d
{
public:
	Prop(std::string objectName);
	~Prop() override;

	void Initialize() override;
	void Update(float deltaTime) override;

#ifdef _DEBUG
	void DebugGui() override;
#endif // _DEBUG

	// 使用するモデル名を設定する(Initialize()より前に呼ぶこと。未設定時は"Cube")
	void SetModelName(const std::string& modelName) { modelName_ = modelName; }

	/// <summary>ランタンなど発光する小物用のポイントライトを設定する（Initialize() より前に呼ぶこと）</summary>
	/// <param name="offset">オブジェクト原点からのローカルオフセット（エンジン座標系。回転・スケールに追従する）</param>
	void SetLight(const Vector3& color, const Vector3& offset, float intensity, float radius, float decay) {
		hasLight_ = true;
		lightColor_ = color;
		lightOffset_ = offset;
		lightIntensity_ = intensity;
		lightRadius_ = radius;
		lightDecay_ = decay;
	}

private:
	std::string modelName_ = "Cube";

	// ── オプションのポイントライト（ランタンなど）──
	bool hasLight_ = false;
	DynamicPointLight* light_ = nullptr; // 所有権は LightManager
	Vector3 lightColor_ = { 1.0f, 0.75f, 0.4f };
	Vector3 lightOffset_ = { 0.0f, 0.0f, 0.0f };
	float lightIntensity_ = 1.5f;
	float lightRadius_ = 8.0f;
	float lightDecay_ = 1.0f;
};
