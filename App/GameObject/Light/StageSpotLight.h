#pragma once
#include "World3D/Object/Object3d.h"

class DynamicSpotLight;

/// <summary>
/// レベルエディタで配置するスポットライト（天井の隙間から差し込む光など）。
/// 描画物は持たず、自分の位置からローカルの -Y 方向（回転させると向きが変わる）へ円錐状に照らす。
///
/// 外側の角度で光が消え、内側の角度までは最も明るい（どちらも光軸からの半角・度）。
/// particleLighting が正なら埃（LitByLights のパーティクル）を照らし、volumetric が正なら空気中に散る。
/// パラメータは SceneBuilder が Initialize() より前に SetParams で設定する。
/// </summary>
class StageSpotLight : public Object3d
{
public:
	struct Params {
		Vector3 color{ 0.8f, 0.88f, 1.0f };
		float intensity = 4.0f;
		float distance = 22.0f;     // 届く距離[m]
		float decay = 0.012f;       // 1/(1 + decay·d²) の減衰
		float outerAngle = 12.0f;   // 外側の半角[度]。ここで光が消える
		float innerAngle = 7.0f;    // 内側の半角[度]。ここまでは最も明るい
		float particleLighting = 1.0f; // 埃を照らす倍率（0 で照らさない）
		float volumetric = 1.0f;       // 空気中に散る倍率（0 で散らない）
	};

	StageSpotLight(std::string objectName);
	~StageSpotLight() override;

	void Initialize() override;
	void Update(float deltaTime) override;

	void SetParams(const Params& params) { params_ = params; ApplyLightParams(); }
	const Params& GetParams() const { return params_; }
	// 編集用。書き換えたら ApplyLightParams() を呼ぶこと
	Params& GetParamsRef() { return params_; }

	/// <summary>現在のパラメータを実体のライトへ反映する（未生成なら何もしない）</summary>
	void ApplyLightParams();

	/// <summary>照らす向き（ワールド、正規化済み）。ローカルの -Y を回転させたもの</summary>
	Vector3 GetWorldDirection();
	/// <summary>光源の位置（ワールド）</summary>
	Vector3 GetWorldPosition();

private:
	DynamicSpotLight* light_ = nullptr; // 所有権は LightManager
	Params params_{};
};
