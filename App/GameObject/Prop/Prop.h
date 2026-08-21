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
#endif // _DEBUG

	// 使用するモデル名は Object3d::SetModelName()。未設定なら Initialize() で "Cube" になる

	/// <summary>ランタンなど発光する小物用のポイントライトを設定する</summary>
	/// <param name="offset">オブジェクト原点からのローカルオフセット（エンジン座標系。回転・スケールに追従する）</param>
	void SetLight(const Vector3& color, const Vector3& offset, float intensity, float radius, float decay) {
		hasLight_ = true;
		lightColor_ = color;
		lightOffset_ = offset;
		lightIntensity_ = intensity;
		lightRadius_ = radius;
		lightDecay_ = decay;
		ApplyLightParams();
	}

	/// <summary>ライトの有無を切り替える（実行中に呼べる。エディタ用）</summary>
	void SetLightEnabled(bool enabled);

	// --- 編集・保存用のアクセッサ ---
	bool HasLight() const { return hasLight_; }
	const Vector3& GetLightColor() const { return lightColor_; }
	const Vector3& GetLightOffset() const { return lightOffset_; }
	float GetLightIntensity() const { return lightIntensity_; }
	float GetLightRadius() const { return lightRadius_; }
	float GetLightDecay() const { return lightDecay_; }

	// 編集用。書き換えたら ApplyLightParams() を呼ぶこと
	Vector3& GetLightColorRef() { return lightColor_; }
	Vector3& GetLightOffsetRef() { return lightOffset_; }
	float& GetLightIntensityRef() { return lightIntensity_; }
	float& GetLightRadiusRef() { return lightRadius_; }
	float& GetLightDecayRef() { return lightDecay_; }

	/// <summary>現在のパラメータを実体のライトへ反映する（未生成なら何もしない）</summary>
	void ApplyLightParams();

protected:
	/// <summary>
	/// 実体のポイントライト（未生成なら nullptr）。
	/// WallTorch のように、派生クラスが毎フレーム明るさや位置を揺らすために使う
	/// </summary>
	DynamicPointLight* GetLight() const { return light_; }

private:
	// ── オプションのポイントライト（ランタンなど）──
	bool hasLight_ = false;
	DynamicPointLight* light_ = nullptr; // 所有権は LightManager
	Vector3 lightColor_ = { 1.0f, 0.75f, 0.4f };
	Vector3 lightOffset_ = { 0.0f, 0.0f, 0.0f };
	float lightIntensity_ = 1.5f;
	float lightRadius_ = 8.0f;
	float lightDecay_ = 1.0f;
};
